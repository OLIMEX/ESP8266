esptool/esptool.py \
	--port /dev/ttyUSB0 \
	--baud 576000 \
	write_flash \
		0x00000  bin/boot_v1.6.bin \
		0x01000  bin/upgrade/user1.2048.new.3.bin \
		0x81000  bin/upgrade/user2.2048.new.3.bin \
		0x100000 bin/blank.bin \
		0x101000 bin/blank.bin \
		0x102000 bin/blank.bin \
		0x103000 bin/blank.bin \
		0x1FC000 bin/esp_init_data_default.bin \
		0x1FE000 bin/blank.bin \
	--flash_size 16m

