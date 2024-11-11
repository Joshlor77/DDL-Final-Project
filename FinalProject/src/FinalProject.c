#include "NoteOutput.h"
#include "Clocking.h"


int main() {
	PLL0StartUpSeq();
	initNoteSystem();

	setChInterval(1, 4000);
	enableCh(1);

    while(1) {

    }
    return 0 ;
}
