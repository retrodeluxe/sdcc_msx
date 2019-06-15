////////////////////////////////////////////////////////////////////////
// Image to MSX tiles
//
// Manuel Martinez (salutte@gmail.com)
//
// FLAGS: -std=gnu++14 -g `pkg-config opencv --cflags --libs` -O3 -lpthread

#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <functional>

using namespace std::chrono_literals;

static const std::vector<cv::Vec3b> colors = { // Note, those are BGR
	{   0,    0,    0},
	{  66,  200,   33},
	{ 120,  220,   94},
	{ 237,   85,   84},
	{ 252,  118,  125},
	{  77,   82,  212},
	{ 245,  235,   66},
	{  84,   85,  252},
	{ 120,  121,  255},
	{  84,  193,  212},
	{ 128,  206,  230},
	{  59,  176,   33},
	{ 186,   91,  201},
	{ 202,  202,  202},
	{ 255,  255,  255}	
};

std::array<int, 256> sRGB2lin;
std::array<int,4096> lin2sRGB;

static void initsRGB() {
	
	
	double a = 0.0003;
	for (size_t i=0; i<256; i++) {
		
		double srgb = (i+0.5)/255.999999999;
		double lin = 0;
		if (srgb <= 0.0405) {
			lin = srgb/12.92;
		} else {
			lin = std::pow((srgb+0.055)/(1.055),2.4);
		}
		sRGB2lin[i]=lin*4095.999999999;
	}
	
	for (size_t i=0; i<4096; i++) {

		double lin = (i+0.5)/4095.999999999;
		double srgb = 0;
		if (lin <= 0.0031308) {
			srgb = lin*12.92;
		} else {
			srgb = 1.055*std::pow(lin,1/2.4)-0.055;
		}
		lin2sRGB[i]=srgb*255.999999999;
	}
}

static cv::Vec3b sRGBadd(cv::Vec3b a, cv::Vec3b b) {
	
	return {
		lin2sRGB[(sRGB2lin[a[0]]+sRGB2lin[b[0]])>>1],
		lin2sRGB[(sRGB2lin[a[1]]+sRGB2lin[b[1]])>>1],
		lin2sRGB[(sRGB2lin[a[2]]+sRGB2lin[b[2]])>>1],
	};
}

static cv::Mat3b sRGBadd(cv::Mat3b a, cv::Mat3b b) {
	
	for (int i=0; i<a.rows; i++)
		for (int j=0; j<a.cols; j++)
			a(i,j) = sRGBadd(a(i,j),b(i,j));

	return a;
}

std::vector<std::array<cv::Vec3b,2>> palette;
std::vector<uint8_t> paletteCode;
std::map<uint8_t,std::array<cv::Vec3b,2>> paletteMap;

typedef std::array<uint8_t,8>  U8x8;
struct Tile {
	
	U8x8 pattern;
	U8x8 color;

	cv::Mat3b img() const {
				
		cv::Mat3b ret(8,8);
		for (size_t i=0; i<8; i++) {
			auto &p = paletteMap[color[i]];
					
			for (size_t j=0; j<8; j++) 
				ret(i,7-j) = p[!!(pattern[i] & (1<<j))];
		}
		return ret;
	}
	
	Tile(const U8x8 &pattern, const U8x8 &color) : pattern(pattern), color(color) {}
	 
	Tile(uint8_t p=0, uint8_t c=0) {
		pattern.fill(p);
		color.fill(c);
	}
	
	bool operator<(const Tile &rhs) const { 
		if (pattern==rhs.pattern) return color < rhs.color;
		return pattern < rhs.pattern;
	}
};

auto compmse = [](const cv::Vec3b &p0, const cv::Vec3b &p1, const cv::Vec3b &p2) {
	
	return	(p0[0]+p1[0]-2*p2[0])*(p0[0]+p1[0]-2*p2[0]) +
			(p0[1]+p1[1]-2*p2[1])*(p0[1]+p1[1]-2*p2[1]) +
			(p0[2]+p1[2]-2*p2[2])*(p0[2]+p1[2]-2*p2[2]);

	/*double mse = 
			std::pow(std::abs(p0[0]+p1[0]-2*p2[0]),2) +
			std::pow(std::abs(p0[1]+p1[1]-2*p2[1]),2) +
			std::pow(std::abs(p0[2]+p1[2]-2*p2[2]),2);

	double mae = 
			std::pow(std::abs(p0[0]+p1[0]-2*p2[0]),1) +
			std::pow(std::abs(p0[1]+p1[1]-2*p2[1]),1) +
			std::pow(std::abs(p0[2]+p1[2]-2*p2[2]),1);

	double msqrte = 
			std::pow(std::abs(p0[0]+p1[0]-2*p2[0]),0.5) +
			std::pow(std::abs(p0[1]+p1[1]-2*p2[1]),0.5) +
			std::pow(std::abs(p0[2]+p1[2]-2*p2[2]),0.5);

	return mae*/;
};

auto bgr2yuv = [](const cv::Vec3b p) {
	
	uint8_t y = (29*p[0]+150*p[1]+77*p[2])>>8;
	return cv::Vec3b(y, (128*256+125*(p[0]-y))>>8, (128*256+224*(p[2]-y))>>8);
};

auto compyuv = [](cv::Vec3b p0, cv::Vec3b p1, cv::Vec3b p2) {
	
	p0 = bgr2yuv(sRGBadd(p0,p1));
	p2 = bgr2yuv(p2);
	
	double color2gray_ratio = 10;
	
	return	(p0[0]-p2[0])*(p0[0]-p2[0]) +
			color2gray_ratio*(p2[0]/256.)*(p0[1]-p2[1])*(p0[1]-p2[1]) +
			color2gray_ratio*(p2[0]/256.)*(p0[2]-p2[2])*(p0[2]-p2[2]);

	return	std::abs(p0[0]-p2[0]) +
			color2gray_ratio*(p2[0]/256.)*std::abs(p0[1]-p2[1]) +
			color2gray_ratio*(p2[0]/256.)*std::abs(p0[2]-p2[2]);


	

};

auto comp = compyuv;

static double lineScore(const cv::Vec3b *estimate, const cv::Vec3b *target) {

	double res = 0;
	for (int i=0; i<8; i++)
		res = res + comp(estimate[i],estimate[i],target[i]);
	
//	for (int i=0; i<7; i++)
//		res = res + comp(estimate[i],target[i]);
		
	return res;
}

static double tileScore(const cv::Mat3b &estimate, const cv::Mat3b &target) {
	
	double res = 0;
	for (int i=0; i<8; i++)
		res = res + lineScore(&estimate(i,0), &target(i,0));

	return res;
}

static std::pair<cv::Mat2i, std::map<size_t, Tile>> quantize( const cv::Mat3b src, cv::Mat3b target) {
		
	std::map<size_t, Tile> tiles;
	for (size_t i=0; i<colors.size()-1; i++)
		tiles[i] = Tile(0,240+i+1);
	tiles[tiles.size()] = Tile(255,254);
	
	cv::Mat2i tileMap(src.rows/8,src.cols/8);
	for (int i=0; i<src.rows; i+=8) {
		
		std::cerr << i << " out of " << src.rows << std::endl;
		for (int j=0; j<src.cols; j+=8) {

			cv::Vec2i selectedTiles;
			double bestErrorTile = 1e100;

			Tile bestTile;
			for (int t=-1; t<15; t++) { // Selects base tile;

				Tile tile;
				double errorTile = 0;
				for (int ii=0; ii<8; ii++) { // Selects row;
					
					double bestErrorLine = 1e100;
					for (size_t p=0; p<palette.size(); p++) {

						double errorLine = 0;
						uint8_t pattern = 0;
						for (int jj=0; jj<8; jj++) {
							
							double v0 = comp((t!=-1?colors[t]:palette[p][0]), palette[p][0], src(i+ii,j+jj));
							double v1 = comp((t!=-1?colors[t]:palette[p][1]), palette[p][1], src(i+ii,j+jj));
							if (v0<v1) {
								pattern = (pattern<<1);
								errorLine += v0;
							} else {
								pattern = (pattern<<1) + 1;
								errorLine += v1;
							}
						}
						
						if (errorLine<bestErrorLine) {
							bestErrorLine = errorLine;
							tile.pattern[ii] = pattern; 
							tile.color[ii] = paletteCode[p];
						}
					}
					errorTile += bestErrorLine;
				}
				
				if (errorTile<bestErrorTile) {
					bestErrorTile = errorTile;
					bestTile = tile;
					selectedTiles[1] = t;
				} 
			}
			
			
			
			selectedTiles[0] = tiles.size(); 
			if (selectedTiles[1] == -1) selectedTiles[1] = selectedTiles[0];
			tiles[tiles.size()] = bestTile;
			target(cv::Rect(j,i,8,8)) = 0.5*tiles[selectedTiles[0]].img() + 0.5*tiles[selectedTiles[1]].img();
		}
	}
	
	{
		
		std::map<Tile, size_t> tt;
		for (auto &t : tiles) 
			tt[t.second]++;
			
		std::cout << "Size: " << tt.size() << " " << tiles.size() << std::endl;
	}

	return std::make_pair(tileMap, tiles);

}

static std::pair<cv::Mat2i, std::map<size_t, Tile>> quantizeBasic( const cv::Mat3b src, cv::Mat3b target) {

	std::map<size_t, Tile> tiles;

	for (size_t i=0; i<colors.size()-1; i++)
		tiles[i] = Tile(0,240+i+1);
	tiles[tiles.size()] = Tile(255,254);
	
	cv::Mat2i tileMap(src.rows/8,src.cols/8);
	for (int i=0; i<src.rows; i+=8) {
		
		std::cerr << i << " out of " << src.rows << std::endl;
		for (int j=0; j<src.cols; j+=8) {

			cv::Vec2i selectedTiles;
			double bestErrorTile = 1e100;

			Tile bestTile;
			for (int t=-1; t<0; t++) { // Selects base tile;

				Tile tile;
				double errorTile = 0;
				for (int ii=0; ii<8; ii++) { // Selects row;
					
					double bestErrorLine = 1e100;
					for (size_t p=0; p<palette.size(); p++) {

						double errorLine = 0;
						uint8_t pattern = 0;
						for (int jj=0; jj<8; jj++) {
							
							double v0 = comp((t!=-1?colors[t]:palette[p][0]), palette[p][0], src(i+ii,j+jj));
							double v1 = comp((t!=-1?colors[t]:palette[p][1]), palette[p][1], src(i+ii,j+jj));
							if (v0<v1) {
								pattern = (pattern<<1);
								errorLine += v0;
							} else {
								pattern = (pattern<<1) + 1;
								errorLine += v1;
							}
						}
						
						if (errorLine<bestErrorLine) {
							bestErrorLine = errorLine;
							tile.pattern[ii] = pattern; 
							tile.color[ii] = paletteCode[p];
						}
					}
					errorTile += bestErrorLine;
				}
				
				if (errorTile<bestErrorTile) {
					bestErrorTile = errorTile;
					bestTile = tile;
					selectedTiles[1] = t;
				} 
			}
			
			selectedTiles[0] = tiles.size(); 
			if (selectedTiles[1] == -1) selectedTiles[1] = selectedTiles[0];
			tiles[tiles.size()] = bestTile;
			target(cv::Rect(j,i,8,8)) = 0.5*tiles[selectedTiles[0]].img() + 0.5*tiles[selectedTiles[1]].img();
		}
	}
	
	{
		
		std::map<Tile, size_t> tt;
		for (auto &t : tiles) 
			tt[t.second]++;
			
		std::cout << "Size: " << tt.size() << " " << tiles.size() << std::endl;
	}

	return std::make_pair(tileMap, tiles);

}

static std::pair<cv::Mat2i, std::map<size_t, Tile>> quantizeBlur( const cv::Mat3b src, cv::Mat3b target) {
		
	std::map<size_t, Tile> tiles;

	for (size_t i=0; i<colors.size()-1; i++)
		tiles[i] = Tile(0,240+i+1);
	tiles[tiles.size()] = Tile(255,254);
	
	cv::Mat2i tileMap(src.rows/8,src.cols/8);
	for (int i=0; i<src.rows; i+=8) {
		
		std::cerr << i << " out of " << src.rows << std::endl;
		for (int j=0; j<src.cols; j+=8) {

			cv::Vec2i selectedTiles;
			double bestErrorTile = 1e100;

			Tile bestTile;
			for (int t=-1; t<15; t++) { // Selects base tile;

				Tile tile;
				double errorTile = 0;
				for (int ii=0; ii<8; ii++) { // Selects row;
					
					double bestErrorLine = 1e100;
					for (size_t p=0; p<palette.size(); p++) {

						double errorLine = 0;
						uint8_t pattern = 0;
						for (int jj=0; jj<8; jj++) {
							
							double v0 = comp((t!=-1?colors[t]:palette[p][0]), palette[p][0], src(i+ii,j+jj));
							double v1 = comp((t!=-1?colors[t]:palette[p][1]), palette[p][1], src(i+ii,j+jj));
							
							cv::Vec3b inter = 
								0.5*(v1/(v0+v1+1e-10)) * ( cv::Vec3d(palette[p][0]) + cv::Vec3d(t!=-1?colors[t]:palette[p][0])) +
								0.5*(v0/(v0+v1+1e-10)) * ( cv::Vec3d(palette[p][1]) + cv::Vec3d(t!=-1?colors[t]:palette[p][1]));
								
							errorLine += 1*comp(inter, inter, src(i+ii,j+jj));
							
							if ((rand()&0xFFFF)/double(0xFFFF) <v1/(v0+v1+1e-10)) {
								pattern = (pattern<<1);
								errorLine += v0;
							} else {
								pattern = (pattern<<1) + 1;
								errorLine += v1;
							}
						}
						
						if (errorLine<bestErrorLine) {
							bestErrorLine = errorLine;
							tile.pattern[ii] = pattern; 
							tile.color[ii] = paletteCode[p];
						}
					}
					errorTile += bestErrorLine;
				}
				
				if (errorTile<bestErrorTile) {
					bestErrorTile = errorTile;
					bestTile = tile;
					selectedTiles[1] = t;
				} 
			}
			
			
			
			selectedTiles[0] = tiles.size(); 
			if (selectedTiles[1] == -1) selectedTiles[1] = selectedTiles[0];
			tiles[tiles.size()] = bestTile;
			target(cv::Rect(j,i,8,8)) = 0.5*tiles[selectedTiles[0]].img() + 0.5*tiles[selectedTiles[1]].img();
		}
	}
	
	{
		
		std::map<Tile, size_t> tt;
		for (auto &t : tiles) 
			tt[t.second]++;
			
		std::cout << "Size: " << tt.size() << " " << tiles.size() << std::endl;
	}

	return std::make_pair(tileMap, tiles);

}




static std::pair<cv::Mat2i, std::map<size_t, Tile>> quantizeFull( const cv::Mat3b src, cv::Mat3b target) {

	std::map<size_t, Tile> tiles;
	cv::Mat2i tileMap(src.rows/8,src.cols/8);

	{ // Find best pair of tiles for each 8x8 block
		std::map<Tile, size_t> differentTiles;
		for (size_t i=0; i<colors.size()-1; i++)
			differentTiles[tiles[i] = Tile(0,240+i+1)] = i;
		differentTiles[tiles[colors.size()-1] = Tile(255,254)] = colors.size()-1;
		
		for (int i=0; i<src.rows; i+=8) { 
			
			std::cerr << i << " out of " << src.rows << std::endl;
			for (int j=0; j<src.cols; j+=8) {

				cv::Vec2i &selectedTiles = tileMap(i/8,j/8);
				double bestErrorTile = 1e100;

				Tile tile0, tile1;
				double errorTile = 0;
				for (int ii=0; ii<8; ii++) { // Selects row;
					
					double bestErrorLine = 1e100;
					for (size_t p0=0; p0<palette.size(); p0++) {
					for (size_t p1=p0; p1<palette.size(); p1++) {

						double errorLine = 0;
						uint8_t pattern0 = 0, pattern1 = 0;
						for (int jj=0; jj<8; jj++) {
							
							double v00 = 0.95*comp(palette[p0][0], palette[p1][0], src(i+ii,j+jj));
							double v01 = 0.96*comp(palette[p0][0], palette[p1][1], src(i+ii,j+jj));
							double v10 = 0.97*comp(palette[p0][1], palette[p1][0], src(i+ii,j+jj));
							double v11 = 0.98*comp(palette[p0][1], palette[p1][1], src(i+ii,j+jj));
							
							double minError = std::min(std::min(v00,v01),std::min(v10,v11));
							errorLine += minError;
							
							if        (v00 == minError) {
								pattern0 = (pattern0<<1);
								pattern1 = (pattern1<<1);
							} else if (v01 == minError) {
								pattern0 = (pattern0<<1);
								pattern1 = (pattern1<<1) + 1;
							} else if (v10 == minError) {
								pattern0 = (pattern0<<1) + 1;
								pattern1 = (pattern1<<1);
							} else if (v11 == minError) {
								pattern0 = (pattern0<<1) + 1;
								pattern1 = (pattern1<<1) + 1;
							}
						}
						
						if (errorLine<bestErrorLine) {
							bestErrorLine = errorLine;
							tile0.pattern[ii] = pattern0; 
							tile1.pattern[ii] = pattern1;
							tile0.color[ii] = paletteCode[p0];
							tile1.color[ii] = paletteCode[p1];
						}
					}
					}
					errorTile += bestErrorLine;
				}
				
				if (not differentTiles.count(tile0))
					differentTiles[tile0] = tiles.size();	
				selectedTiles[0] = differentTiles[tile0];
				tiles[selectedTiles[0]] = tile0;
				
				if (not differentTiles.count(tile1))
					differentTiles[tile1] = tiles.size();	
				selectedTiles[1] = differentTiles[tile1];
				tiles[selectedTiles[1]] = tile1;
			}
		}
	}
		
	{	// Summarize in order to have maximum 256 tiles
		std::map<Tile, size_t> tt;
		tt.clear();
		for (auto &t : tiles) 
			tt[t.second]++;

		std::cout << "Size: " << tt.size() << " " << tiles.size() << std::endl;
		
		while (tt.size()>2560) {

			std::map<double, std::pair<size_t, size_t>> tileComp;
			for (auto &t0 : tiles) 
				for (auto &t1 : tiles) 
					if (t0.first<t1.first)
						tileComp[cv::norm(cv::Mat3d(t1.second.img())-cv::Mat3d(t0.second.img()))] = std::make_pair(t0.first,t1.first);
				
/*			for (auto &&tm : tileMap) 
				if (tm[0]==tileComp.begin()->second.second)
					tm[0]=tileComp.begin()->second.first;

			for (auto &&tm : tileMap) 
				if (tm[1]==tileComp.begin()->second.second)
					tm[1]=tileComp.begin()->second.first;*/
			
			tiles.erase(tileComp.begin()->second.second);


			{
				std::vector<std::pair<int,int>> affectedBlocks;
				std::map<size_t, std::reference_wrapper<Tile>> affectedTiles;

				for (int i=0; i<tileMap.rows; i++) {
					for (int j=0; j<tileMap.cols; j++) {
						if (tileMap(i,j)[0]==tileComp.begin()->second.second or 
							tileMap(i,j)[1]==tileComp.begin()->second.second) {
							
							if (tileMap(i,j)[0]==tileComp.begin()->second.second)
								tileMap(i,j)[0]=tileComp.begin()->second.first;

							if (tileMap(i,j)[1]==tileComp.begin()->second.second)
								tileMap(i,j)[1]=tileComp.begin()->second.first;
							
							if (tileMap(i,j)[0]>15) {
								affectedTiles.emplace(tileMap(i,j)[0], tiles[tileMap(i,j)[0]]);
								affectedBlocks.emplace_back(i,j);
							}
							if (tileMap(i,j)[1]>15) {
								affectedTiles.emplace(tileMap(i,j)[1], tiles[tileMap(i,j)[1]]);
								affectedBlocks.emplace_back(i,j);
							}
						}
					}
				}
				
				std::cerr << affectedBlocks.size() << " " << affectedTiles.size() << std::endl;	
				
				
				for (auto &&at : affectedTiles) {
					
					size_t tileIdx = at.first;
					Tile &tile = at.second.get();
					
					double bestErrorLine = 1e100;
					
					for (int ii=0; ii<8; ii++) { // Select tile row
							
						for (size_t p0=0; p0<palette.size(); p0++) {

							double errorLine = 0;
							uint8_t pattern = 0;
							
							for (int jj=0; jj<8; jj++) { // Select tile col
								
								double v0 = 0., v1 = 0.;

								for (auto &&block : affectedBlocks) {
									
									auto colora0 = tiles[tileMap(block.first,block.second)[0]].img()(ii,jj);
									auto colorb0 = tiles[tileMap(block.first,block.second)[1]].img()(ii,jj);
									auto colora1 = colora0;
									auto colorb1 = colorb0;
									
									if (tileMap(block.first,block.second)[0]==tileIdx) {
										colora0 = palette[p0][0];
										colora1 = palette[p0][1];
									}
									if (tileMap(block.first,block.second)[1]==tileIdx) {
										colorb0 = palette[p0][0];
										colorb1 = palette[p0][1];
									}
										
									v0 += comp(colora0, colorb0, src(block.first*8+ii,block.second*8+jj));
									v1 += comp(colora1, colorb1, src(block.first*8+ii,block.second*8+jj));
								}
									
								double minError = std::min(v0,v1);
								errorLine += minError;
									
								if        (v0 == minError) {
									pattern = (pattern<<1);
								} else if (v1 == minError) {
									pattern = (pattern<<1) + 1;
								}
							}
							
							if (errorLine<bestErrorLine) {
								bestErrorLine = errorLine;
								tile.pattern[ii] = pattern; 
								tile.color[ii] = paletteCode[p0];
							}
						}
					}
				}					
			}




			tt.clear();
			for (auto &t : tiles) 
				tt[t.second]++;
				
			std::cout << "Size: " << tt.size() << " " << tiles.size() << " " << tileComp.begin()->first <<  std::endl;

		};
	}
	
	
	
	for (int i=0; i<src.rows; i+=8)
		for (int j=0; j<src.cols; j+=8)
//			target(cv::Rect(j,i,8,8)) = 0.5*tiles[tileMap(i/8,j/8)[0]].img() + 0.5*tiles[tileMap(i/8,j/8)[1]].img();
			sRGBadd(tiles[tileMap(i/8,j/8)[0]].img(),tiles[tileMap(i/8,j/8)[1]].img()).copyTo(target(cv::Rect(j,i,8,8)));

	return std::make_pair(tileMap, tiles);

}


typedef uint8_t TileIdx;

typedef struct {
	
	uint8_t rows, cols;
	uint8_t nCustomTiles;
	Tile *customTiles;
	TileIdx *data[2];
} Image;
	
int main(int argc, char *argv[]) {
	
	initsRGB();

	{
		std::vector<cv::Vec3b> C;
		for (size_t i=0; i<colors.size(); i++) {
			for (size_t j=i+1; j<colors.size(); j++) {
				auto newColor = sRGBadd(colors[i],colors[j]);
				bool found = false;
				for (auto &c : C)
					if (cv::norm(cv::Vec3d(c)-cv::Vec3d(newColor))<40.001)
						found = true;
				if (not found)
					C.push_back(sRGBadd(colors[i],colors[j]));
			}
		}
		std::cout << C.size() << std::endl;	
		
	}
	
	std::cerr << lin2sRGB[sRGB2lin[255]*0.5 + sRGB2lin[255]*0.5] << std::endl;
	std::cerr << lin2sRGB[sRGB2lin[202]*0.5 + sRGB2lin[255]*0.5] << std::endl;
	std::cerr << lin2sRGB[sRGB2lin[  0]*0.5 + sRGB2lin[255]*0.5] << std::endl;

	std::cerr << lin2sRGB[sRGB2lin[202]*0.5 + sRGB2lin[202]*0.5] << "<" << std::endl;
	std::cerr << lin2sRGB[sRGB2lin[  0]*0.5 + sRGB2lin[202]*0.5] << std::endl;
	std::cerr << lin2sRGB[sRGB2lin[  0]*0.5 + sRGB2lin[  0]*0.5] << std::endl;


	if (argc!=2) {
		std::cout << "Use: ./mkImage <file.png>" << std::endl;
		return -1;
	}

	cv::Mat3b img = cv::imread(argv[1]);
	
	cv::Mat3d imgd = cv::Mat3d(img);
	//imgd = (imgd-202)*1.5 + 202;
	//imgd = (0.7*imgd)+0.3*255;
	//for (auto &c : imgd) c[0]=250;
	img = cv::Mat3b(imgd);
		
	img = img(cv::Rect(0,0,img.cols&0xFFF8,img.rows&0xFFF8)).clone();
	
	std::cerr << img.rows << " " << img.cols << std::endl;
		
	for (size_t i=0; i<colors.size(); i++) {
		for (size_t j=i+1; j<colors.size(); j++) {
			palette.emplace_back(std::array<cv::Vec3b,2>{colors[i],colors[j]});
			paletteCode.emplace_back(16*(j  + 1) + i  + 1);			
			paletteMap[paletteCode.back()] = palette.back();
		}
	}
	
	
	cv::Mat3b work = img.clone();
	cv::Mat3b target;

	cv::resize(work, target, cv::Size(), 4, 4, cv::INTER_NEAREST);
	cv::imshow("original", target);
	
	
	
/*	auto res = quantizeBlur(img, work);
	cv::imwrite("out.png",work);	
	std::cerr << res.first.rows * res.first.cols << " " << res.second.size()*sizeof(Tile) << std::endl;
	cv::resize(work, target, cv::Size(), 4, 4, cv::INTER_NEAREST);
	cv::imshow("q_blur", target);	
	cv::waitKey(0);

	res = quantizeBlur2(img, work);
	cv::imwrite("out.png",work);	
	std::cerr << res.first.rows * res.first.cols << " " << res.second.size()*sizeof(Tile) << std::endl;
	cv::resize(work, target, cv::Size(), 4, 4, cv::INTER_NEAREST);
	cv::imshow("q_blur", target);	
	cv::waitKey(0);

	res = quantize(img, work);
	cv::imwrite("out.png",work);
	
	std::cerr << res.first.rows * res.first.cols << " " << res.second.size()*sizeof(Tile) << std::endl;
	cv::resize(work, target, cv::Size(), 4, 4, cv::INTER_NEAREST);
	cv::imshow("q_noblur", target);
	
	cv::waitKey(0);

	res = quantizeBasic(img, work);
	std::cerr << res.first.rows * res.first.cols << " " << res.second.size()*sizeof(Tile) << std::endl;
	cv::resize(work, target, cv::Size(), 4, 4, cv::INTER_NEAREST);
	cv::imshow("q_basic", target);
	
	cv::waitKey(0);
	*/

	{
		auto res = quantizeFull(img, work);

		cv::imwrite("out.png",work);
		
		std::cout << cv::norm(work) << std::endl;
		std::cout << cv::norm(img) << std::endl;
		
		std::cerr << res.first.rows * res.first.cols << " " << res.second.size()*sizeof(Tile) << std::endl;
		cv::resize(work, target, cv::Size(), 4, 4, cv::INTER_NEAREST);
		cv::imshow("q_full", target);
		
		cv::waitKey(0);
	}

	


/*	std::string name = argv[1];
	name = name.substr(name.rfind('-')+1);
	name = name.substr(0,name.size()-4);
	name = "font_"+name;
	

	std::cout << "// Font file generated from:" << name.substr(name.rfind('-')+1) << std::endl;
	std::cout << "#include <stdint.h>" << std::endl;
	std::cout << "const uint8_t " << name << "[128][8] = {" << std::endl;
	for (int i=0; i<128; i++) {
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
*/	
	return 0;
}