var Messages = require('./messages');

const PROGRAM_INTERVAL = 1000 * 60 * 3; // 3 minutes

var OpenFestProgram = [
	{
		from:  new Date (2015, 10, 7, 10, 30),
		to:    new Date (2015, 10, 7, 10, 45),
		hall:  'Зала "България"',
		event: 'Официално откриване'
	},
	{
		from:  new Date (2015, 10, 7, 10, 45),
		to:    new Date (2015, 10, 7, 11, 45),
		hall:  'Зала "България"',
		event: 'The Tyranny of Everyday Things (Aral Balkan)'
	},
	
	/* ================================================================== */
	
	{
		from:  new Date (2015, 10, 7, 12, 00),
		to:    new Date (2015, 10, 7, 12, 45),
		hall:  'Зала "България"',
		event: 'Свободен софтуер - що е то (Боян Кроснов)'
	},
	{
		from:  new Date (2015, 10, 7, 12, 00),
		to:    new Date (2015, 10, 7, 12, 45),
		hall:  'Камерна зала',
		event: 'MySQL Plugin Development (Георги Кодинов)'
	},
	{
		from:  new Date (2015, 10, 7, 12, 00),
		to:    new Date (2015, 10, 7, 12, 45),
		hall:  'Студио "Музика"',
		event: 'Email сигурност (Явор Папазов)'
	},
	
	/* ================================================================== */
	
	{
		from:  new Date (2015, 10, 7, 13, 00),
		to:    new Date (2015, 10, 7, 13, 45),
		hall:  'Зала "България"',
		event: 'КлинКлин: силата на колективната медия (Кирил Беспалов)'
	},
	{
		from:  new Date (2015, 10, 7, 13, 00),
		to:    new Date (2015, 10, 7, 13, 45),
		hall:  'Камерна зала',
		event: 'Backing Up Thousands of Containers (Marian Marinov)'
	},
	{
		from:  new Date (2015, 10, 7, 13, 00),
		to:    new Date (2015, 10, 7, 13, 45),
		hall:  'Студио "Музика"',
		event: 'Contributor Agreements (Илияна Лилова)'
	},
	
	/* ================================================================== */
	
	{
		from:  new Date (2015, 10, 7, 15, 00),
		to:    new Date (2015, 10, 7, 15, 45),
		hall:  'Зала "България"',
		event: 'Designers vs. Developers (Hollie Lubbock)'
	},
	{
		from:  new Date (2015, 10, 7, 15, 00),
		to:    new Date (2015, 10, 7, 15, 45),
		hall:  'Камерна зала',
		event: 'C++ към JavaScript с Emscripten (Борислав Станимиров)'
	},
	{
		from:  new Date (2015, 10, 7, 15, 00),
		to:    new Date (2015, 10, 7, 15, 45),
		hall:  'Студио "Музика"',
		event: 'The Yocto Project (Леон Анави, Радослав Колев)'
	},
	
	/* ================================================================== */
	
	{
		from:  new Date (2015, 10, 7, 16, 00),
		to:    new Date (2015, 10, 7, 16, 45),
		hall:  'Зала "България"',
		event: 'strace: Monitoring The Kernel-User-Space Conversation (Michael Kerrisk)'
	},
	{
		from:  new Date (2015, 10, 7, 16, 00),
		to:    new Date (2015, 10, 7, 16, 45),
		hall:  'Камерна зала',
		event: 'Building Universal Applications with Angular 2 (Минко Гечев)'
	},
	{
		from:  new Date (2015, 10, 7, 16, 00),
		to:    new Date (2015, 10, 7, 16, 45),
		hall:  'Студио "Музика"',
		event: 'FOSS Information Security Applications (Димитър Шалварджиев)'
	},
	
	/* ================================================================== */
	
	{
		from:  new Date (2015, 10, 7, 17, 00),
		to:    new Date (2015, 10, 7, 17, 45),
		hall:  'Зала "България"',
		event: 'Physical Computing и Internet Of Things с отворен хардуер и софтуер (Цветан Узунов)'
	},
	{
		from:  new Date (2015, 10, 7, 17, 00),
		to:    new Date (2015, 10, 7, 17, 45),
		hall:  'Камерна зала',
		event: 'Free And Open Source Software In European Public Administrations (Gijs Hillenius)'
	},
	{
		from:  new Date (2015, 10, 7, 17, 00),
		to:    new Date (2015, 10, 7, 17, 45),
		hall:  'Студио "Музика"',
		event: 'Нов подход за помощ на хора с увреден слух (Георги Иванов)'
	},
	
	/* ================================================================== */
	
	{
		from:  new Date (2015, 10, 7, 18, 00),
		to:    new Date (2015, 10, 7, 18, 45),
		hall:  'Зала "България"',
		event: 'Тригодишната ми дъщеря е по-добър програмист от мен (Красимир Цонев)'
	},
	{
		from:  new Date (2015, 10, 7, 18, 00),
		to:    new Date (2015, 10, 7, 18, 45),
		hall:  'Камерна зала',
		event: 'Lightning talks'
	},
	
	/* ================================================================== */
	/* ================================================================== */
	/* ================================================================== */
	
	{
		from:  new Date (2015, 10, 8, 10, 00),
		to:    new Date (2015, 10, 8, 10, 45),
		hall:  'Зала "България"',
		event: 'Listening to Berlin’s Open Communities (Mark Rendeiro)'
	},
	{
		from:  new Date (2015, 10, 8, 10, 00),
		to:    new Date (2015, 10, 8, 10, 45),
		hall:  'Камерна зала',
		event: 'Асинхронен Python (Евгени Кунев)'
	},
	{
		from:  new Date (2015, 10, 8, 10, 00),
		to:    new Date (2015, 10, 8, 10, 45),
		hall:  'Студио "Музика"',
		event: 'Конференции с Jitsi Meet (Ясен Праматаров)'
	},
	
	/* ================================================================== */
	
	{
		from:  new Date (2015, 10, 8, 11, 00),
		to:    new Date (2015, 10, 8, 11, 45),
		hall:  'Зала "България"',
		event: 'First Lessons in Control Theory (Hirotsugu Asari)'
	},
	{
		from:  new Date (2015, 10, 8, 11, 00),
		to:    new Date (2015, 10, 8, 11, 45),
		hall:  'Камерна зала',
		event: 'Програмиране за най-начинаещи и школите в София (Явор Никифоров)'
	},
	{
		from:  new Date (2015, 10, 8, 11, 00),
		to:    new Date (2015, 10, 8, 11, 45),
		hall:  'Студио "Музика"',
		event: 'C++ на стероиди (Николай Николов)'
	},
	
	/* ================================================================== */
	
	{
		from:  new Date (2015, 10, 8, 12, 00),
		to:    new Date (2015, 10, 8, 12, 45),
		hall:  'Зала "България"',
		event: 'Електронна идентификация (Божидар Божанов)'
	},
	{
		from:  new Date (2015, 10, 8, 12, 00),
		to:    new Date (2015, 10, 8, 12, 45),
		hall:  'Камерна зала',
		event: 'CryptoParty Must Die (Marie Gutbub)'
	},
	{
		from:  new Date (2015, 10, 8, 12, 00),
		to:    new Date (2015, 10, 8, 12, 45),
		hall:  'Студио "Музика"',
		event: 'Enterprise Integration Patterns with Apache Camel (Явор Янков)'
	},
	
	/* ================================================================== */
	
	{
		from:  new Date (2015, 10, 8, 14, 00),
		to:    new Date (2015, 10, 8, 14, 45),
		hall:  'Зала "България"',
		event: 'Звук и движение в контекста на интерактивността (Атанас Динчев)'
	},
	{
		from:  new Date (2015, 10, 8, 14, 00),
		to:    new Date (2015, 10, 8, 14, 45),
		hall:  'Камерна зала',
		event: 'Програмиране на "compute kernels" за x86-64 (Боян Кроснов)'
	},
	{
		from:  new Date (2015, 10, 8, 14, 00),
		to:    new Date (2015, 10, 8, 14, 45),
		hall:  'Студио "Музика"',
		event: 'Университетът - резерват за светли умове (Димитър Коджабашев)'
	},
	
	/* ================================================================== */
	
	{
		from:  new Date (2015, 10, 8, 15, 00),
		to:    new Date (2015, 10, 8, 15, 45),
		hall:  'Зала "България"',
		event: 'Електронно гласуване (Божидар Божанов)'
	},
	{
		from:  new Date (2015, 10, 8, 15, 00),
		to:    new Date (2015, 10, 8, 15, 45),
		hall:  'Камерна зала',
		event: 'Using seccomp to Limit the Kernel Attack Surface (Michael Kerrisk)'
	},
	{
		from:  new Date (2015, 10, 8, 15, 00),
		to:    new Date (2015, 10, 8, 15, 45),
		hall:  'Студио "Музика"',
		event: 'GraphQL (Радослав Станков)'
	},
	
	/* ================================================================== */
	
	{
		from:  new Date (2015, 10, 8, 16, 00),
		to:    new Date (2015, 10, 8, 16, 45),
		hall:  'Зала "България"',
		event: 'New Money (Vladimir Dzhuvinov)'
	},
	{
		from:  new Date (2015, 10, 8, 16, 00),
		to:    new Date (2015, 10, 8, 16, 45),
		hall:  'Камерна зала',
		event: 'TChannel, или как Uber прави RPC (Любомир Райков)'
	},
	{
		from:  new Date (2015, 10, 8, 16, 00),
		to:    new Date (2015, 10, 8, 16, 45),
		hall:  'Студио "Музика"',
		event: 'Образование 3.0'
	},
	
	/* ================================================================== */
	
	{
		from:  new Date (2015, 10, 8, 17, 00),
		to:    new Date (2015, 10, 8, 17, 45),
		hall:  'Зала "България"',
		event: 'Collaboration In Open Source - How Does It Really Work? (Otto Kekalainen)'
	},
	
	/* ================================================================== */
	
	{
		from:  new Date (2015, 10, 8, 17, 45),
		to:    new Date (2015, 10, 8, 18, 00),
		hall:  'Зала "България"',
		event: 'Закриване'
	},
	
];

function dateTimePad(n) {
	if (n < 10) {
		return '0'+n;
	}
	return n;
}

function DisplayProgram() {
	var now = new Date();
	var queue = 0;
	
	OpenFestProgram.forEach(
		function (entry) {
			if (now.getDate() != entry.from.getDate()) {
				return;
			}
			
			if (now >= entry.to) {
				return;
			}
			
			if (now > entry.from) {
				Messages.display(
					{
						node: '*',
						text: entry.hall+' - '+entry.event,
						r: 1, g: 0, b: 0
					}
				);
				queue++;
			} else if (queue < 6) {
				Messages.display(
					{
						node: '*',
						text: 
							dateTimePad(entry.from.getHours())+':'+dateTimePad(entry.from.getMinutes())+' '+
							entry.hall+' - '+entry.event
						,
						r: 0, g: 1, b: 0
					}
				);
				queue++;
			}
		}
	);
}

setInterval(
	function () {
		DisplayProgram();
	},
	PROGRAM_INTERVAL
);
