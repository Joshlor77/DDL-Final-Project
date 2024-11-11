#include "NoteOutput.h"
#include "Clocking.h"


int main() {
	PLL0StartUpSeq();
	initNoteSystem();

	setChInterval(0, 8000);
	enableCh(0);

    while(1) {

    }
    return 0 ;
}
