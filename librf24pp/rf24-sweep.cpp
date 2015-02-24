#include <stdio.h>
#include <librf24/librf24.hpp>

using namespace librf24;


INITIALIZE_EASYLOGGINGPP

static FILE *Gplt;

void sweepDone(LibRF24Transfer &t)
{
	LibRF24SweepTransfer &s = (LibRF24SweepTransfer &) t;
	
	for (int i=0; i<128; i++)
		fprintf(Gplt, "%d %d\n", i, s.getObserved(i));

	fprintf(Gplt,"e\n");
	fprintf(Gplt,"replot\n");
	std::cout << ">>>>>>>>>>> " << s.getObserved(91) << "\n" ; 
	t.submit();
}

int main(int argc, char** argv)
{	
	START_EASYLOGGINGPP(argc, argv);

	LibRF24Adaptor *a = new LibRF24LibUSBAdaptor(); 

	LibRF24SweepTransfer t(*a, 7);
	t.setCallback(sweepDone);
	t.submit();	

	Gplt = popen("gnuplot", "w");
	setlinebuf(Gplt);
	fprintf(Gplt, "set autoscale\n"); 
	fprintf(Gplt, "plot '-' w lp\n"); 

	while (1) { 
		a->loopOnce();
	}
}
