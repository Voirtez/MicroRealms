#include "stm32l031lib.h"
#include "realm.h"
int main()
{
	initSerial();
	while(1)
	{
		runGame();
	}
}
