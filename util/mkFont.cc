////////////////////////////////////////////////////////////////////////
// PNG to Font encoder
//
// Manuel Martinez (salutte@gmail.com)
//
// FLAGS: -std=c++14 -g `pkg-config opencv4 --cflags --libs`

#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <vector>

int main(int argc, char *argv[]) {
	
	if (argc!=2) {
		std::cout << "Use: ./mkFont <file.png>" << std::endl;
		return -1;
	}

	cv::Mat3b img = cv::imread(argv[1]);

	cv::Mat1b small(8*128,8);
	for (int i=0; i<8; i++)
		for (int j=0; j<16; j++)
			for (int k=0; k<8; k++)
				for (int l=0; l<8; l++)
					small((i*16+j)*8+k,l) 
						= 255*!!(img((i*8+k),(j*8+l))[0]>128);

	uint8_t font[128][8];
	for (int i=0; i<128; i++)
		for (int j=0; j<8; j++)
			for (int k=0; k<8; k++)
				font[i][j] = (font[i][j] << 1) + !!small(i*8+j,k);

	std::string name = argv[1];
	name = name.substr(name.rfind('/')+1);
	name = name.substr(0,name.size()-4);
	

	std::cout << "// Font file generated from:" << name.substr(name.rfind('-')+1) << std::endl;
	std::cout << "typedef unsigned char uint8_t;" << std::endl;
	std::cout << "const uint8_t " << name << "[96][8] = {" << std::endl;
	for (int i=32; i<128; i++) {
		std::cout << std::hex << "\t {";
		for (int j=0; j<8; j++) {
			std::cout << (j?",":"");
			printf("0x%02X",font[i][j]);
		}
		std::cout << "} " << (i==128?' ':',');
//		if (i>31 and i<127) std::cout << " \\\\ '" << char(i) << "' ";
		std::cout << std::endl;
	}
	std::cout << "};" << std::endl;
	
	return 0;
}
