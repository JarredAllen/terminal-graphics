/*
 * This file displays png images in the terminal.
 *
 * The executable should be called by ./<filename> <image>.png
 */
#include <iostream>
#include <fstream>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <unistd.h>
#include "lodepng.h"
// lodepng is a png library for C++ that I found online, available here:
// https://github.com/lvandeve/lodepng/

std::array<unsigned int, 256> colors;

std::string execCommand(char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

inline int combine_rgb(int r, int g, int b) {
    return b + (g<<8) + (r<<16);
}

/*
 * Returns the difference between two colors, formatted as RRGGBB hex integers
 */
int color_difference(int c1, int c2) {
    int db = (c1 & 0xFF) - (c2 & 0xFF);
    int dg = (c1 >> 8 & 0xFF) - (c2 >> 8 & 0xFF);
    int dr = (c1 >> 16 & 0xFF) - (c2 >> 16 & 0xFF);
    return db*db+dg*dg+dr*dr;
}

/*
 * Returs the closest color code to the given RRGGBB hex code.
 */
int closest_color(int color) {
    int best = -1;
    int distance = 1000000;
    for (int i=0; i<256; i++) {
        if (distance > color_difference(color, colors[i])) {
            distance = color_difference(color, colors[i]);
            best = i;
        }
    }
    return best;
}

/*
 * Decodes the file and outputs the image colors.
 * colors, width, and height are outparameters
 */
void decode_file(std::string file, int**& colors, unsigned& width, unsigned& height) {
    std::vector<unsigned char> data;
    if (lodepng::decode(data, width, height, file, LCT_RGB)) {
        throw std::runtime_error("could not open file");
    }
    colors = new int*[height];
    for (int i=0; i<height; i++) {
        colors[i] = new int[width];
        for (int j=0; j<width; j++) {
            colors[i][j] = closest_color(combine_rgb(data[3*(width*i+j)+0],
                                                     data[3*(width*i+j)+1],
                                                     data[3*(width*i+j)+2]));
        }
    }
}

/*
 * Decodes the file, and then shrinks the image to width by height.
 * If the image is sufficiently small, then it pads the image with black
 *
 * Imgout specifies the area to be written to and must be already made and
 * large enough to store the image.
 */
void decode_and_resize(std::string file, int** imgout, int rows, int cols) {
    std::vector<unsigned char> data;
    unsigned int width, height;
    if (lodepng::decode(data, width, height, file, LCT_RGB)) {
        throw std::runtime_error("could not open file: "+file);
    }
    if (rows <= height) {
        if (cols <= width) {
            for (int i=0; i<rows; i++) {
                for (int j=0; j<cols; j++) {
                    int x1 = height * i / rows;
                    int x2 = height * (i+1) / rows;
                    int y1 = width * j / cols;
                    int y2 = width * (j+1) / cols;
                    int closest = -1;
                    unsigned int cost = -1;
                    for (int index = 0; index < 256; index++) {
                        unsigned int curr_cost = 0;
                        for (int a=x1; a<x2; a++) {
                            for (int b=y1; b<y2; b++) {
                                curr_cost += color_difference(combine_rgb(data[3*(a*width+b)+0],
                                                                          data[3*(a*width+b)+1],
                                                                          data[3*(a*width+b)+2]),
                                                              colors[index]);
                            }
                        }
                        if (curr_cost < cost) {
                            cost = curr_cost;
                            closest = index;
                        }
                    }
                    imgout[i][j] = closest;
                }
            }
        }
        else {
        }
    }
    else {
        //Just add the rows normally
    }
}

/*
 * Displays the image containing the given color codes, with dimensions of
 * rows and cols. It is your responsibility to ensure that it fits in the
 * terminal.
 */
void display(int rows, int cols, int** colors) {
    for (int i=0; i<rows; i++) {
        std::cout << '\n';
        for (int j=0; j<cols; j++) {
            std::cout << "\u001b[38;5;" << colors[i][j] << "m█";
        }
    }
    std::cout.flush();
}

int main(int argc, char** argv) {
    if (argc == 1) {
        std::cout << "You didn't give me an image :(\n";
        return 1;
    }
    // Load the color choices that I have from the file
    std::ifstream color_file;
    color_file.open("colors.txt");
    char* line = new char[15];
    for (int i=0; i<256; i++) {
        color_file.getline(line, 15);
        int index = 0;
        while (line[index++] != ':');
        colors[i] = std::stoi(line+index, nullptr, 16);
    }
    // Now I can do stuff
    int rows = std::stoi(execCommand("tput lines"));
    int cols = std::stoi(execCommand("tput cols"));
    int** colors = new int*[rows];
    for (int i=0; i<rows; i++) {
        colors[i] = new int[cols];
    }
    decode_and_resize(argv[1], colors, rows, cols);
    display(rows, cols, colors);
    usleep(1000000);
    std::cout << "\n";
}

