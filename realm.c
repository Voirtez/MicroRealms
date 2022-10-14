
/*
Copyright (C) 2014  Frank Duignan

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "realm.h"
#include "stm32l031lib.h"
#include <stdint.h>
#include <stm32l031xx.h>
#include "musical_notes.h"

#define CPU_FREQ 16000000

// Find types: h(ealth),s(trength),m(agic),g(old),w(eapon)
static const char FindTypes[]={'h','s','m','g','w'};

static volatile uint32_t SoundDuration = 0;


// The following arrays define the bad guys and 
// their battle properies - ordering matters!
// Baddie types : O(gre),T(roll),D(ragon),H(ag)
static const char Baddies[]={'O','T','D','H'};
// The following is 4 sets of 4 damage types
static const byte WeaponDamage[]={10,10,5,25,10,10,5,25,10,15,5,15,5,5,2,10};
#define ICE_SPELL_COST 10
#define FIRE_SPELL_COST 20
#define LIGHTNING_SPELL_COST 30
static const byte FreezeSpellDamage[]={10,20,5,0};
static const byte FireSpellDamage[]={20,10,5,0};
static const byte LightningSpellDamage[]={15,10,25,0};
static const byte BadGuyDamage[]={10,10,15,5};
static int GameStarted = 0;
static tPlayer thePlayer;
static tRealm theRealm;


void ROn(void);
void ROff(void);
void YOn(void);
void YOff(void);
void GOn(void);
void GOff(void);

void DamageLED(void);


void SysTick_Handler(void);
void playNote(uint32_t Frequency, uint32_t Duration);

void start_seq(void);

void banner(void);
void gameOver_ascii(void);

void intro_music(void);
void victory(void);
void gameOver(void);


void dragon_ascii(void);
void ogre_ascii(void);
void troll_ascii(void);
void hag_ascii(void);

__attribute__((noreturn)) void runGame(void)  
{
	char ch;
	pinMode(GPIOA, 0, 1); // make GPIOA_0 an output
	pinMode(GPIOA, 1, 1); // make GPIOA_1 an output
	pinMode(GPIOA, 3, 1); // make GPIOA_3 an output
	pinMode(GPIOA, 4, 1); // make GPIOA_4 an output
	
	SysTick->LOAD = 4000;   // 16MHz / 16000 = 1kHz
	SysTick->CTRL = 7; 			// enable systick counting and interrupts, use core clock
	
	start_seq();
	
	banner(); // draw game banner
	intro_music();
	
	eputs("MicroRealms on the STM32L031\r\n");	
	showHelp();		
	while(GameStarted == 0)
	{
		
		showGameMessage("Press S to start a new game\r\n");
		ch = getUserInput();			
		
		if ( (ch == 'S') || (ch == 's') )
			GameStarted = 1;
	}
	
	initRealm(&theRealm);	
	initPlayer(&thePlayer,&theRealm);
	showPlayer(&thePlayer);
	showRealm(&theRealm,&thePlayer);
	showGameMessage("Press H for help");
	
	RCC->IOPENR = RCC->IOPENR | 3;
	
	while (1)
	{
		ch = getUserInput();
		ch = ch | 32; // enforce lower case
		switch (ch) {
			case 'h' : {
				showHelp();
				break;
			}
			case 'w' : {
				showGameMessage("North");
				step('n',&thePlayer,&theRealm);
				
				break;
			}
			case 's' : {
				showGameMessage("South");
				step('s',&thePlayer,&theRealm);
				break;

			}
			case 'd' : {
				showGameMessage("East");
				step('e',&thePlayer,&theRealm);
				break;
			}
			case 'a' : {
				showGameMessage("West");
				step('w',&thePlayer,&theRealm);
				break;
			}
			case '#' : {		
				if (thePlayer.wealth)		
				{
					showRealm(&theRealm,&thePlayer);
					thePlayer.wealth--;
				}
				else
					showGameMessage("No gold!");
				break;
			}
			case 'p' : {				
				showPlayer(&thePlayer);
				break;
			}
		} // end switch
	} // end while
}
void step(char Direction,tPlayer *Player,tRealm *Realm)
{
	uint8_t new_x, new_y;
	new_x = Player->x;
	new_y = Player->y;
	byte AreaContents;
	switch (Direction) {
		case 'n' :
		{
			if (new_y > 0)
				new_y--;
			break;
		}
		case 's' :
		{
			if (new_y < MAP_HEIGHT-1)
				new_y++;
			break;
		}
		case 'e' :
		{
			if (new_x <  MAP_WIDTH-1)
				new_x++;
			break;
		}
		case 'w' :
		{
			if (new_x > 0)
				new_x--;
			break;
		}		
	}
	AreaContents = Realm->map[new_y][new_x];
	if ( AreaContents == '*')
	{
		showGameMessage("A rock blocks your path.");
		return;
	}
	Player->x = new_x;
	Player->y = new_y;
	int Consumed = 0;
	switch (AreaContents)
	{
		
		// const char Baddies[]={'O','T','B','H'};
		case 'O' :{
			ogre_ascii(); // draw ogre
			showGameMessage("A smelly green Ogre appears before you");
			Consumed = doChallenge(Player,0);
			break;
		}
		case 'T' :{
			troll_ascii(); // draw troll
			showGameMessage("An evil troll challenges you");
			Consumed = doChallenge(Player,1);
			break;
		}
		case 'D' :{
			dragon_ascii(); // draw dragon
			showGameMessage("A smouldering Dragon blocks your way !");
			Consumed = doChallenge(Player,2);
			break;
		}
		case 'H' :{
			hag_ascii(); // draw hag
			showGameMessage("A withered hag cackles at you wickedly");
			Consumed = doChallenge(Player,3);
			break;
		}
		case 'h' :{
			showGameMessage("You find an elixer of health");
			setHealth(Player,Player->health+10);
			Consumed = 1;		
			break;
			
		}
		case 's' :{
			showGameMessage("You find a potion of strength");
			Consumed = 1;
			setStrength(Player,Player->strength+1);
			break;
		}
		case 'g' :{
			showGameMessage("You find a shiny golden nugget");
			Player->wealth++;			
			Consumed = 1;
			break;
		}
		case 'm' :{
			showGameMessage("You find a magic charm");
			Player->magic++;						
			Consumed = 1;
			break;
		}
		case 'w' :{
			Consumed = addWeapon(Player,(int)random(MAX_WEAPONS-1)+1);
			showPlayer(Player);
			break;			
		}
		case 'X' : {
			// Player landed on the exit
			eputs("A door! You exit into a new realm");
			setHealth(Player,100); // maximize health
			initRealm(&theRealm);
			showRealm(&theRealm,Player);
		}
	}
	if (Consumed)
		Realm->map[new_y][new_x] = '.'; // remove any item that was found
}
int doChallenge(tPlayer *Player,int BadGuyIndex)
{
	char ch;
	char Damage;
	const byte *dmg;
	int BadGuyHealth = 100;
	eputs("Press F to fight");
	ch = getUserInput() | 32; // get user input and force lower case
	if (ch == 'f')
	{
		eputs("\r\nChoose action");
		while ( (Player->health > 0) && (BadGuyHealth > 0) )
		{
			eputs("\r\n");
			// Player takes turn first
			if (Player->magic > ICE_SPELL_COST)
				eputs("(I)CE spell");
			if (Player->magic > FIRE_SPELL_COST)
				eputs("(F)ire spell");
			if (Player->magic > LIGHTNING_SPELL_COST)
				eputs("(L)ightning spell");
			if (Player->Weapon1)
			{
				eputs("(1)Use ");
				eputs(getWeaponName(Player->Weapon1));
			}	
			if (Player->Weapon2)
			{
				eputs("(2)Use ");
				eputs(getWeaponName(Player->Weapon2));
			}
			eputs("(P)unch");
			ch = getUserInput();
			switch (ch)
			{
				case 'i':
				case 'I':
				{
					eputs("FREEZE!");
					Player->magic -= ICE_SPELL_COST;
					BadGuyHealth -= FreezeSpellDamage[BadGuyIndex]+random(10);
					zap();
					break;
				}
				case 'f':
				case 'F':
				{
					eputs("BURN!");
					Player->magic -= FIRE_SPELL_COST;
					BadGuyHealth -= FireSpellDamage[BadGuyIndex]+random(10);
					zap();
					break;
				}
				case 'l':
				case 'L':
				{
					eputs("ZAP!");
					Player->magic -= LIGHTNING_SPELL_COST;
					BadGuyHealth -= LightningSpellDamage[BadGuyIndex]+random(10);
					zap();
					break;
				}
				case '1':
				{
					dmg = WeaponDamage+(Player->Weapon1<<2)+BadGuyIndex;
					eputs("Take that!");
					BadGuyHealth -= *dmg + random(Player->strength);
					setStrength(Player,Player->strength-1);
					break;
				}
				case '2':
				{
					dmg = WeaponDamage+(Player->Weapon2<<2)+BadGuyIndex;
					eputs("Take that!");
					BadGuyHealth -= *dmg + random(Player->strength);
					setStrength(Player,Player->strength-1);
					break;
				}
				case 'p':
				case 'P':
				{
					eputs("Thump!");
					BadGuyHealth -= 1+random(Player->strength);
					setStrength(Player,Player->strength-1);
					break;
				}
				default: {
					eputs("You fumble. Uh oh");
				}
			}
			// Bad guy then gets a go 
			
			if (BadGuyHealth < 0)
				BadGuyHealth = 0;
			Damage = (uint8_t)(BadGuyDamage[BadGuyIndex]+(int)random(5));
			setHealth(Player,Player->health - Damage);
			
			// blink yellow LED when player takes damage
			DamageLED();
			
			eputs("Health: you "); printDecimal(Player->health);
			eputs(", them " );printDecimal((uint32_t)BadGuyHealth);
			eputs("\r\n");
		}
		if (Player->health == 0)
		{ // You died
			eputs("You are dead. Press Reset to restart\r\n");
			
			// play game over sequence
			gameOver();
				
			
			while(1);
		}
		else
		{ // You won!
			Player->wealth = (uint8_t)(50 + random(50));			
			showGameMessage("You win! Their gold is yours\r\n");
			
			// play victory sequence
			victory();
			
			return 1;
		}
		
	}
	else
	{
		showGameMessage("Our 'hero' chickens out\r\n");
		return 0;
	}
}
int addWeapon(tPlayer *Player, int Weapon)
{
	char c;
	eputs("You stumble upon ");
	switch (Weapon)
	{
		case 1:
		{	
			eputs("a mighty axe");
			break;
		}
		case 2:
		{	
			eputs("a sword with mystical runes");
			break;
		}
		case 3:
		{	
			eputs("a bloody flail");
			break;
		}		
		default:
			printDecimal((uint32_t)Weapon);
	}
	if ( (Player->Weapon1) && (Player->Weapon2) )
	{
		// The player has two weapons already.
		showPlayer(Player);
		eputs("You already have two weapons\r\n");		
		eputs("(1) drop Weapon1, (2) for Weapon2, (0) skip");
		c = getUserInput();
		eputchar(c);
		switch(c)
		{
			case '0':{
				return 0; // don't pick up
			}
			case '1':{
				Player->Weapon1 = (uint8_t)Weapon;
				break;
			}
			case '2':{
				Player->Weapon2 = (uint8_t)Weapon;
				break;
			}
		}
	}
	else
	{
		if (!Player->Weapon1)
		{
			Player->Weapon1 = (uint8_t)Weapon;	
		}
		else if (!Player->Weapon2)
		{
			Player->Weapon2 = (uint8_t)Weapon;
		}
	}	
	return 1;
}
const char *getWeaponName(int index)
{
	switch (index)
	{
		case 0:return "Empty"; 
		case 1:return "Axe";
		case 2:return "Sword"; 
		case 3:return "Flail"; 
	}
	return "Unknown";
}

void setHealth(tPlayer *Player,int health)
{
	if (health > 100)
		health = 100;
	if (health < 0)
		health = 0;
	Player->health = (uint8_t)health;
}	
void setStrength(tPlayer *Player, byte strength)
{
	if (strength > 100)
		strength = 100;
	Player->strength = strength;
}
void initPlayer(tPlayer *Player,tRealm *Realm)
{
	// get the player name
	int index=0;
	byte x,y;
	char ch=0;
	// Initialize the player's attributes
	eputs("Enter the player's name: ");
	while ( (index < MAX_NAME_LEN) && (ch != '\n') && (ch != '\r'))
	{
		ch = getUserInput();
		if ( ch > '0' ) // strip conrol characters
		{
			
			Player->name[index++]=ch;
			eputchar(ch);
		}
	}
	Player->name[index]=0; // terminate the name
	setHealth(Player,100);
	Player->strength=(uint8_t)(50+random(50));
	Player->magic=(uint8_t)(50+random(50));	
	Player->wealth=(uint8_t)(10+random(10));
	Player->Weapon1 = 0;
	Player->Weapon2 = 0;
	// Initialize the player's location
	// Make sure the player does not land
	// on an occupied space to begin with
	do {
		x=(uint8_t)random(MAP_WIDTH);
		y=(uint8_t)random(MAP_HEIGHT);
		
	} while(Realm->map[y][x] != '.');
	Player->x=x;
	Player->y=y;
}
void showPlayer(tPlayer *Player)
{
	eputs("\r\nName: ");
	eputs(Player->name);
	eputs("health: ");
	printDecimal(Player->health);
	eputs("\r\nstrength: ");
	printDecimal(Player->strength);
	eputs("\r\nmagic: ");
	printDecimal(Player->magic);
	eputs("\r\nwealth: ");
	printDecimal(Player->wealth);	
	eputs("\r\nLocation : ");
	printDecimal(Player->x);
	eputs(" , ");
	printDecimal(Player->y);	
	eputs("\r\nWeapon1 : ");
	eputs(getWeaponName(Player->Weapon1));
	eputs(" Weapon2 : ");
	eputs(getWeaponName(Player->Weapon2));
}
void initRealm(tRealm *Realm)
{
	unsigned int x,y;
	unsigned int Rnd;
	// clear the map to begin with
	for (y=0;y < MAP_HEIGHT; y++)
	{
		for (x=0; x < MAP_WIDTH; x++)
		{
			Rnd = random(100);
			
			if (Rnd >= 98) // put in some baddies
				Realm->map[y][x]=	Baddies[random(sizeof(Baddies))];
			else if (Rnd >= 95) // put in some good stuff
				Realm->map[y][x]=	FindTypes[random(sizeof(FindTypes))];
			else if (Rnd >= 90) // put in some rocks
				Realm->map[y][x]='*'; 
			else // put in empty space
				Realm->map[y][x] = '.';	
		}
	}
	
	// finally put the exit to the next level in
	x = random(MAP_WIDTH);
	y = random(MAP_HEIGHT);
	Realm->map[y][x]='X';
}
void showRealm(tRealm *Realm,tPlayer *Player)
{
	int x,y;
	eputs("\r\nThe Realm:\r\n");	
	for (y=0;y<MAP_HEIGHT;y++)
	{
		for (x=0;x<MAP_WIDTH;x++)
		{
			
			if ( (x==Player->x) && (y==Player->y))
				eputchar('@');
			else
				eputchar(Realm->map[y][x]);
		}
		eputs("\r\n");
	}
	eputs("\r\nLegend\r\n");
	eputs("(T)roll, (O)gre, (D)ragon, (H)ag, e(X)it\r\n");
	eputs("(w)eapon, (g)old), (m)agic, (s)trength\r\n");
	eputs("@=You\r\n");
}
void showHelp()
{

	eputs("Help\r\n");
	eputs("W,A,S,D : go North, West, South, East\r\n");
	eputs("# : show map (cost: 1 gold piece)\r\n");
	eputs("(H)elp\r\n");
	eputs("(P)layer details\r\n");
	
}

void showGameMessage(char *Msg)
{
	eputs(Msg);
	eputs("\r\nReady\r\n");	
}
char getUserInput()
{
	char ch = 0;
	
	while (ch == 0)
		ch = egetchar();
	return ch;
}
unsigned random(unsigned range)
{
	// Implementing my own version of modulus
	// as it is a lot smaller than the library version
	// To prevent very long subtract loops, the
	// size of the value returned from prbs has been
	// restricted to 8 bits.
	unsigned Rvalue = (prbs()&0xff);
	while (Rvalue >= range)
		Rvalue -= range; 
	return Rvalue;
}
void zap()
{

}
uint32_t prbs()
{
	// This is an unverified 31 bit PRBS generator
	// It should be maximum length but this has not been verified
	static unsigned long shift_register=0xa5a5a5a5;
	unsigned long new_bit=0;
	static int busy=0; // need to prevent re-entrancy here
	if (!busy)
	{
		busy=1;
		new_bit= ((shift_register & (1<<27))>>27) ^ ((shift_register & (1<<30))>>30);
		new_bit= ~new_bit;
		new_bit = new_bit & 1;
		shift_register=shift_register << 1;
		shift_register=shift_register | (new_bit);
		busy=0;
	}
	return shift_register & 0x7ffffff; // return 31 LSB's
}

// functions to trun LEDs on and/or off
void ROn(void)
{
	GPIOA->ODR = GPIOA->ODR | (1u << 1);
	
} // end ROn()

void ROff(void)
{
	GPIOA->ODR = GPIOA->ODR & ~(1u << 1);
	
} // end ROff()

void YOn(void)
{
	GPIOA->ODR = GPIOA->ODR | (1u << 3);
	
} // end YOn()

void YOff(void)
{
	GPIOA->ODR = GPIOA->ODR & ~(1u << 3);
	
} // end YOff()

void GOn(void)
{
	GPIOA->ODR = GPIOA->ODR | (1u << 4);
	
} // end GOn()

void GOff(void)
{
	GPIOA->ODR = GPIOA->ODR & ~(1u << 4);
	
} // end GOff()


//function to blink yellow LED
// used when player takes damage
void DamageLED(void)
{
	YOn();
	delay(1000000);
	YOff();
	
}


// fucntion for start sequence
void start_seq(void)
{
	delay(2000000);
	GOn();
	playNote(A4, 200);
	delay(1000000);
	GOff();
	delay(1000000);
	YOn();
	playNote(B4, 200);
	delay(1000000);
	YOff();
	delay(1000000);
	ROn();
	playNote(C4, 200);
	delay(1000000);
	ROff();
	delay(1000000);
	ROn();
	playNote(C4, 200);
	delay(1000000);
	YOn();
	playNote(B4, 200);
	delay(1000000);
	GOn();
	playNote(A4, 200);
	delay(1000000);
	GOff();
	playNote(A4, 200);
	delay(1000000);
	YOff();
	playNote(B4, 200);
	delay(1000000);
	ROff();
	playNote(C4, 200);
	
} // end start_seq


//function to draw game banner
void banner(void)
{
	eputs("   _____  .__                   __________              .__ \r\n");
	eputs("  /     \\ |__| ___________  ____\\______   \\ ____ _____  |  |   _____   ______ \r\n");
	eputs(" /  \\ /  \\|  |/ ___\\_  __ \\/  _ \\|       _// __ \\__   \\ |  |  /     \\ /  ___/ \r\n");
	eputs("/    Y    \\  \\  \\___|  | \\(  <_> )    |   \\  ___/ / __ \\|  |_|  Y Y  \\___  \\ \r\n");
	eputs("\\____|__  /__|\\___  >__|   \\____/|____|_  /\\___  >____  /____/__|_|  /____  > \r\n");
	eputs("        \\/        \\/                    \\/     \\/     \\/           \\/     \\/ \r\n");
	
}


// function to draw game over text
void gameOver_ascii(void)
{
	eputs("  ________                        ________ \r\n");
	eputs(" /  _____/_____    _____   ____   \\_____  \\___  __ ___________ \r\n");
	eputs("/   \\  ___\\__  \\  /     \\_/ __ \\   /   |   \\  \\/ // __ \\_  __ \\ \r\n");
	eputs("\\    \\_\\  \\/ __ \\|  Y Y  \\  ___/  /    |    \\   /\\  ___/|  | \\/ \r\n");
	eputs(" \\______  (____  /__|_|  /\\___  > \\_______  /\\_/  \\___  >__| \r\n");
	eputs("        \\/     \\/      \\/     \\/          \\/          \\/ \r\n");
}

// function to draw dragon
void dragon_ascii(void)
{
	eputs("                __        _\r\n");
	eputs("              _/  \\    _(\\(o\r\n");
	eputs("             /     \\  /  _  ^^^o\r\n");
	eputs("            /   !   \\/  ! '!!!v'\r\n");
	eputs("           !  !  \\ _' ( \\____\r\n");
	eputs("           ! . \\ _!\\   \\===^\\) \r\n");
	eputs("            \\ \\_!  / __!\r\n");
	eputs("             \\!   /    \\ \r\n");
	eputs("       (\\_      _/   _\\ )\r\n");
	eputs("        \\ ^^--^^ __-^ /(__\r\n");
	eputs("         ^^----^^    \"^--v'\r\n");
	
}


// fucntion to draw ogre
void ogre_ascii(void)
{
	eputs("                           __,='`````'=/__\r\n");
	eputs("                          '//  (o) \\(o) \\ `'         _,-,\r\n");
	eputs("                          //|     ,_)   (`\\      ,-'`_,-\\ \r\n");
	eputs("                        ,-~~~\\  `'==='  /-,      \\==```` \\__\r\n");
	eputs("                       /        `----'     `\\     \\       \\/ \r\n");
	eputs("                    ,-`                  ,   \\  ,.-\\       \\ \r\n");
	eputs("                   /      ,               \\,-`\\`_,-`\\_,..--'\\ \r\n");
	eputs("                  ,`    ,/,              ,>,   )     \\--`````\\ \r\n");
	eputs("                  (      `\\`---'`  `-,-'`_,<   \\      \\_,.--'`\r\n");
	eputs("                   `.      `--. _,-'`_,-`  |    \\ \r\n");
	eputs("                    [`-.___   <`_,-'`------(    / \r\n");
	eputs("                    (`` _,-\\   \\ --`````````|--` \r\n");
	eputs("                     >-`_,-`\\,-` ,          | \r\n");
	eputs("                   <`_,'     ,  /\\          / \r\n");
	eputs("                    `  \\/\\,-/ `/  \\/`\\_/V\\_/ \r\n");
	eputs("                       (  ._. )    ( .__. ) \r\n");
	eputs("                       |      |    |      | \r\n");
	eputs("                        \\,---_|    |_---./ \r\n");
	eputs("                        ooOO(_)    (_)OOoo \r\n");
                          
}


// function to draw troll
void troll_ascii(void)
{
	eputs("      -. -. `.  / .-' _.'  _\r\n");
	eputs("     .--`. `. `| / __.-- _' `\r\n");
	eputs("    '.-.  \\  \\ |  /   _.' `_\r\n");
	eputs("    .-. \\  `  || |  .' _.-' `.\r\n");
	eputs("  .' _ \\ '  -    -'  - ` _.-.\r\n");
	eputs("   .' `. %%%%%   | %%%%% _.-.`-\r\n");
	eputs(" .' .-. ><(@)> ) ( <(@)>< .-.`.\r\n");
	eputs("   ((\"`(   -   | |   -   )'\")) \r\n");
	eputs("  / \\#)\\     (.(_).)    /(#//\\ \r\n");
	eputs(" ' / ) ((  /   | |   \\  )) (`.`.\r\n");
	eputs(" .'  (.) \\ .md88o88bm. / (.) \\)\r\n");
	eputs("   / /| / \\ `Y88888Y' / \\ | \\ \\ \r\n");
	eputs(" .' / O  / `.   -   .' \\  O \\ \\\\ \r\n");
	eputs("  / /(O)/ /| `.___.' | \\\\(O) \\ \r\n");
	eputs("   / / / / |  |   |  |\\  \\  \\ \\ \r\n");
	eputs("   / / // /|  |   |  |  \\  \\ \\ \r\n");
	eputs(" _.--/--/'( ) ) ( ) ) )`\\-\\-\\-._ \r\n");
	eputs("( ( ( ) ( ) ) ( ) ) ( ) ) ) ( ) ) \r\n");
}


// function to draw hag
void hag_ascii(void)
{
	eputs("                                      __,,, \r\n");
	eputs("                                 _.--'    / \r\n");
	eputs("                              .-'        / \r\n");
	eputs("                            .'          | \r\n");
	eputs("                          .'           / \r\n");
	eputs("                         /_____________| \r\n");
	eputs("                       /`~~~~~~~~~~~~~~/ \r\n");
	eputs("                     /`               / \r\n");
	eputs("      ____,....----'/_________________|---....,___ \r\n");
	eputs(",--""`             `~~~~~~~~~~~~~~~~~~`           `""--, \r\n");
	eputs("`'----------------.----,------------------------,-------'` \r\n");
	eputs("               _,'.--. \\                         \\ \r\n");
	eputs("             .'  (o   ) \\                        | \r\n");
	eputs("            c   , '--'  |                        / \r\n");
	eputs("           /    )'-.    \\                       / \r\n");
	eputs("          |  .-;        \\                       | \r\n");
	eputs("          \\_/  |___,'    |                    .-' \r\n");
	eputs("         .---.___|_      |                   / \r\n");
	eputs("        (          `     |               .--' \r\n");
	eputs("         '.         __,-'\\             .' \r\n");
	eputs("           `'-----'`      \\        __.' \r\n");
	eputs("                           `-..--'` \r\n");
}


void playNote(uint32_t Frequency, uint32_t Duration)
{
	SysTick->LOAD = CPU_FREQ / (2 * Frequency);
	SoundDuration = Duration;
}

void SysTick_Handler(void)
{
	if (SoundDuration > 0)
	{
		GPIOA->ODR ^= 1; // Toggle Port A bit 0
		SoundDuration --;
	}
	
}


// function to play intro_music
void intro_music(void)
{
	playNote(D4, 400);
	while(SoundDuration != 0); // Wait
	delay(100000);
	playNote(D4, 200);
	while(SoundDuration != 0); // Wait
	playNote(F4, 100);
	while(SoundDuration != 0); // Wait
	playNote(E4, 200);
	while(SoundDuration != 0); // Wait
	playNote(C4, 200);
	while(SoundDuration != 0); // Wait
	playNote(D4, 200);
	while(SoundDuration != 0); // Wait
	
	playNote(D4, 400);
	while(SoundDuration != 0); // Wait
	delay(100000);
	playNote(D4, 200);
	while(SoundDuration != 0); // Wait
	playNote(F4, 100);
	while(SoundDuration != 0); // Wait
	playNote(E4, 200);
	while(SoundDuration != 0); // Wait
	playNote(C4, 200);
	while(SoundDuration != 0); // Wait
	playNote(D4, 200);
	while(SoundDuration != 0); // Wait
	
	playNote(D4, 400);
	while(SoundDuration != 0); // Wait
	delay(100000);
	playNote(D4, 200);
	while(SoundDuration != 0); // Wait
	playNote(F4, 100);
	while(SoundDuration != 0); // Wait
	playNote(E4, 200);
	while(SoundDuration != 0); // Wait
	playNote(C4, 200);
	while(SoundDuration != 0); // Wait
	playNote(D4, 200);
	
	
}



// function to do victory sequence
void victory(void)
{
	GOn();
	playNote(AS4_Bb4, 100);
	while(SoundDuration != 0); // Wait
	playNote(D4, 100);
	while(SoundDuration != 0); // Wait
	playNote(F4, 400);
	while(SoundDuration != 0); // Wait
	delay(100000);
	playNote(F4, 200);
	while(SoundDuration != 0); // Wait
	playNote(D5, 100);
	while(SoundDuration != 0); // Wait
	playNote(C4, 200);
	while(SoundDuration != 0); // Wait
	playNote(C4, 100);
	while(SoundDuration != 0); // Wait
	playNote(E4, 100);
	while(SoundDuration != 0); // Wait
	playNote(A4, 100);
	while(SoundDuration != 0); // Wait
	delay(100000);
	playNote(G4, 100);
	while(SoundDuration != 0); // Wait
	playNote(C4, 100);
	while(SoundDuration != 0); // Wait
	playNote(E4, 200);
	while(SoundDuration != 0); // Wait
	delay(100000);
	playNote(G4, 100);
	while(SoundDuration != 0); // Wait
	playNote(C4, 100);
	GOff();
	
}

// fucntion to play game over music
void gameOver(void)
{
	ROn();
	gameOver_ascii();
	playNote(A4, 300);
	while(SoundDuration != 0); // Wait
	playNote(C4, 300);
	while(SoundDuration != 0); // Wait
	playNote(A4, 300);
	while(SoundDuration != 0); // Wait
	playNote(D4, 600);
	
}
