#include "NoteOutput.h"


int main() {
	PLL0StartUpSeq();
	initNoteSystem();


	setChInterval(0, 668);
	setChInterval(1, 1000);
	enableCh(0);
	enableCh(1);

    while(1) {

    }
    return 0 ;
}
