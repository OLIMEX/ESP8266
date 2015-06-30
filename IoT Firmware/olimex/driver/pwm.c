/******************************************************************************
* Copyright 2013-2014 Espressif Systems (Wuxi)
*
* FileName: pwm.c
*
* Description: pwm driver
*
* Modification history:
*     2014/5/1, v1.0 create this file.
*******************************************************************************/
#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"

#include "user_interface.h"
#include "driver/pwm.h"

LOCAL struct pwm_param pwm;

LOCAL bool rdy_flg = 0;		//calc finished flag
LOCAL bool update_flg = 0;	//update finished flag
LOCAL bool init_flg = 0;		//first update flag

//define local struct array
LOCAL struct pwm_single_param local_single[PWM_CHANNEL + 1];	//local_single param
LOCAL uint8 local_channel = 0;								//local_channel value

LOCAL struct pwm_single_param pwm_single[PWM_CHANNEL + 1];		//pwm_single param
LOCAL uint8 pwm_channel = 0;									//pwm_channel value

LOCAL struct pwm_single_param saved_single[PWM_CHANNEL + 1];	//saved_single param
LOCAL uint8 saved_channel = 0;								//saved_channel value

LOCAL uint8 pwm_out_io_num[PWM_CHANNEL] = {PWM_0_OUT_IO_NUM,
                                           PWM_1_OUT_IO_NUM, PWM_2_OUT_IO_NUM
                                          };	//each channel gpio number
LOCAL uint8 pwm_current_channel = 0;							//current pwm channel in pwm_tim1_intr_handler
LOCAL uint16 pwm_gpio = 0;									//all pwm gpio bits

//XXX: 0xffffffff/(80000000/16)=35A
#define US_TO_RTC_TIMER_TICKS(t)          \
    ((t) ?                                   \
     (((t) > 0x35A) ?                   \
      (((t)>>2) * ((APB_CLK_FREQ>>4)/250000) + ((t)&0x3) * ((APB_CLK_FREQ>>4)/1000000))  :    \
      (((t) *(APB_CLK_FREQ>>4)) / 1000000)) :    \
     0)

#define FRC1_ENABLE_TIMER  BIT7

//TIMER PREDIVED MODE
typedef enum {
    DIVDED_BY_1 = 0,		//timer clock
    DIVDED_BY_16 = 4,	//divided by 16
    DIVDED_BY_256 = 8,	//divided by 256
} TIMER_PREDIVED_MODE;

typedef enum {			//timer interrupt mode
    TM_LEVEL_INT = 1,	// level interrupt
    TM_EDGE_INT   = 0,	//edge interrupt
} TIMER_INT_MODE;

// sort all channels' h_time,small to big
LOCAL void ICACHE_FLASH_ATTR
pwm_insert_sort(struct pwm_single_param pwm[], uint8 n)
{
    uint8 i;

    for (i = 1; i < n; i++) {
        if (pwm[i].h_time < pwm[i - 1].h_time) {
            int8 j = i - 1;
            struct pwm_single_param tmp;

            os_memcpy(&tmp, &pwm[i], sizeof(struct pwm_single_param));
            os_memcpy(&pwm[i], &pwm[i - 1], sizeof(struct pwm_single_param));

            while (tmp.h_time < pwm[j].h_time) {
                os_memcpy(&pwm[j + 1], &pwm[j], sizeof(struct pwm_single_param));
                j--;

                if (j < 0) {
                    break;
                }
            }

            os_memcpy(&pwm[j + 1], &tmp, sizeof(struct pwm_single_param));
        }
    }
}

void ICACHE_FLASH_ATTR
pwm_start(void)
{
    uint8 i, j;

    // if not first update,save local param to saved param,local_channel to saved_channel
    if (init_flg == 1) {
        for (i = 0; i < local_channel; i++) {
            os_memcpy(&saved_single[i], &local_single[i], sizeof(struct pwm_single_param));
        }

        saved_channel = local_channel;
    }

    rdy_flg = 0;	 //clear rdy_flg before calcing local struct param

    // step 1: init PWM_CHANNEL+1 channels param
    for (i = 0; i < PWM_CHANNEL; i++) {
        uint32 us = pwm.period * pwm.duty[i] / PWM_DEPTH;		//calc  single channel us time
        local_single[i].h_time = US_TO_RTC_TIMER_TICKS(us);	//calc h_time to write FRC1_LOAD_ADDRESS
        local_single[i].gpio_set = 0;							//don't set gpio
        local_single[i].gpio_clear = 1 << pwm_out_io_num[i];	//clear single channel gpio
    }

    local_single[PWM_CHANNEL].h_time = US_TO_RTC_TIMER_TICKS(pwm.period);		//calc pwm.period channel us time
    local_single[PWM_CHANNEL].gpio_set = pwm_gpio;			//set all channels' gpio
    local_single[PWM_CHANNEL].gpio_clear = 0;					//don't clear gpio

    // step 2: sort, small to big
    pwm_insert_sort(local_single, PWM_CHANNEL + 1);			//time sort small to big,
    local_channel = PWM_CHANNEL + 1;							//local channel number is PWM_CHANNEL+1

    // step 3: combine same duty channels
    for (i = PWM_CHANNEL; i > 0; i--) {
        if (local_single[i].h_time == local_single[i - 1].h_time) {
            local_single[i - 1].gpio_set |= local_single[i].gpio_set;
            local_single[i - 1].gpio_clear |= local_single[i].gpio_clear;

            //copy channel j param to channel j-1 param
            for (j = i + 1; j < local_channel; j++) {
                os_memcpy(&local_single[j - 1], &local_single[j], sizeof(struct pwm_single_param));
            }

            local_channel--;
        }
    }

    // step 4: calc delt time
    for (i = local_channel - 1; i > 0; i--) {
        local_single[i].h_time -= local_single[i - 1].h_time;
    }

    // step 5: last channel needs to clean
    local_single[local_channel - 1].gpio_clear = 0;

    // step 6: if first channel duty is 0, remove it
    if (local_single[0].h_time == 0) {

        local_single[local_channel - 1].gpio_set &= ~local_single[0].gpio_clear;
        local_single[local_channel - 1].gpio_clear |= local_single[0].gpio_clear;

        //copy channel i param to channel i-1 param
        for (i = 1; i < local_channel; i++) {
            os_memcpy(&local_single[i - 1], &local_single[i], sizeof(struct pwm_single_param));
        }

        local_channel--;
    }

    // if the first update,copy local  param to pwm  param and copy local_channel to pwm_channel
    if (init_flg == 0) {
        for (i = 0; i < local_channel; i++) {
            os_memcpy(&pwm_single[i], &local_single[i], sizeof(struct pwm_single_param));
        }

        pwm_channel = local_channel;
        init_flg = 1;	//first update finished
        RTC_REG_WRITE(FRC1_LOAD_ADDRESS, US_TO_RTC_TIMER_TICKS(pwm.period));	//first update finished,start
    }

    update_flg = 1;	//update finished
    rdy_flg = 1;		//calc ready

}


/******************************************************************************
* FunctionName : pwm_set_duty
* Description  : set each channel's duty param
* Parameters   : uint8 duty    : 0 ~ PWM_DEPTH
*                uint8 channel : channel index
* Returns      : NONE
*******************************************************************************/
void ICACHE_FLASH_ATTR
pwm_set_duty(uint8 duty, uint8 channel)
{
    if (duty < 1) {
        pwm.duty[channel] = 0;
    } else if (duty >= PWM_DEPTH) {
        pwm.duty[channel] = PWM_DEPTH;
    } else {
        pwm.duty[channel] = duty;
    }
}

/******************************************************************************
* FunctionName : pwm_set_freq
* Description  : set pwm frequency
* Parameters   : uint16 freq : 100hz typically
* Returns      : NONE
*******************************************************************************/
void ICACHE_FLASH_ATTR
pwm_set_freq(uint16 freq)
{
    if (freq > 500) {
        pwm.freq = 500;
    } else if (freq < 1) {
        pwm.freq = 1;
    } else {
        pwm.freq = freq;
    }

    pwm.period = PWM_1S / pwm.freq;
}

/******************************************************************************
* FunctionName : pwm_set_freq_duty
* Description  : set pwm frequency and each channel's duty
* Parameters   : uint16 freq : 100hz typically
*                uint8 *duty : each channel's duty
* Returns      : NONE
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
pwm_set_freq_duty(uint16 freq, uint8 *duty)
{
    uint8 i;

    pwm_set_freq(freq);

    for (i = 0; i < PWM_CHANNEL; i++) {
        pwm_set_duty(duty[i], i);
    }
}

/******************************************************************************
* FunctionName : pwm_get_duty
* Description  : get duty of each channel
* Parameters   : uint8 channel : channel index
* Returns      : NONE
*******************************************************************************/
uint8 ICACHE_FLASH_ATTR
pwm_get_duty(uint8 channel)
{
    return pwm.duty[channel];
}

/******************************************************************************
* FunctionName : pwm_get_freq
* Description  : get pwm frequency
* Parameters   : NONE
* Returns      : uint16 : pwm frequency
*******************************************************************************/
uint16 ICACHE_FLASH_ATTR
pwm_get_freq(void)
{
    return pwm.freq;
}

/******************************************************************************
* FunctionName : pwm_period_timer
* Description  : pwm period timer function, output high level,
*                start each channel's high level timer
* Parameters   : NONE
* Returns      : NONE
*******************************************************************************/
void pwm_tim1_intr_handler(void)
{
    uint8 i = 0;
    RTC_CLR_REG_MASK(FRC1_INT_ADDRESS, FRC1_INT_CLR_MASK);

    if (pwm_current_channel == (pwm_channel - 1)) {

        // if update is not finished,don't copy param,otherwise copy
        if (update_flg == 1) {
            // if calc ready,copy local param, otherwise copy saved param
            if (rdy_flg == 1) {
                for (i = 0; i < local_channel; i++) {
                    os_memcpy(&pwm_single[i], &local_single[i], sizeof(struct pwm_single_param));
                }

                pwm_channel = local_channel;
            } else {
                for (i = 0; i < saved_channel; i++) {
                    os_memcpy(&pwm_single[i], &saved_single[i], sizeof(struct pwm_single_param));
                }

                pwm_channel = saved_channel;
            }
        }

        update_flg = 0;	//clear update flag
        pwm_current_channel = 0;

        gpio_output_set(pwm_single[pwm_channel - 1].gpio_set,
                        pwm_single[pwm_channel - 1].gpio_clear,
                        pwm_gpio,
                        0);

        //if all channels' duty is 0 or 255,set intr period as pwm.period
        if (pwm_channel != 1) {
            RTC_REG_WRITE(FRC1_LOAD_ADDRESS, pwm_single[pwm_current_channel].h_time);
        } else {
            RTC_REG_WRITE(FRC1_LOAD_ADDRESS, US_TO_RTC_TIMER_TICKS(pwm.period));
        }
    } else {
        gpio_output_set(pwm_single[pwm_current_channel].gpio_set,
                        pwm_single[pwm_current_channel].gpio_clear,
                        pwm_gpio, 0);
        pwm_current_channel++;
        RTC_REG_WRITE(FRC1_LOAD_ADDRESS, pwm_single[pwm_current_channel].h_time);
    }
}

/******************************************************************************
* FunctionName : pwm_init
* Description  : pwm gpio, param and timer initialization
* Parameters   : uint16 freq : pwm freq param
*                uint8 *duty : each channel's duty
* Returns      : NONE
*******************************************************************************/
void ICACHE_FLASH_ATTR
pwm_init(uint16 freq, uint8 *duty)
{
    uint8 i;

    RTC_REG_WRITE(FRC1_CTRL_ADDRESS,
                  DIVDED_BY_16
                  | FRC1_ENABLE_TIMER
                  | TM_EDGE_INT);
    //RTC_REG_WRITE(FRC1_LOAD_ADDRESS, 0);

    PIN_FUNC_SELECT(PWM_0_OUT_IO_MUX, PWM_0_OUT_IO_FUNC);
    PIN_FUNC_SELECT(PWM_1_OUT_IO_MUX, PWM_1_OUT_IO_FUNC);
    PIN_FUNC_SELECT(PWM_2_OUT_IO_MUX, PWM_2_OUT_IO_FUNC);

    for (i = 0; i < PWM_CHANNEL; i++) {
        pwm_gpio |= (1 << pwm_out_io_num[i]);
    }

    pwm_set_freq_duty(freq, duty);

    pwm_start();

    ETS_FRC_TIMER1_INTR_ATTACH(pwm_tim1_intr_handler, NULL);
    TM1_EDGE_INT_ENABLE();
    ETS_FRC1_INTR_ENABLE();
}

