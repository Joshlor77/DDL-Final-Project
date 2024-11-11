#include "NoteOutput.h"
#include "Clocking.h"


int main() {
	PLL0StartUpSeq();
	initNoteSystem();

	outputClkPin();

	//setChInterval(0, 33333);
	setChInterval(1, 4000);
	//enableCh(0);
	enableCh(1);

    while(1) {

    }
    return 0 ;
}
