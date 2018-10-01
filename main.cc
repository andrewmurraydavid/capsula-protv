#include "include/led-matrix.h"
#include "include/graphics.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>

#include <time.h>
#include <algorithm>
#include <string>
#include <curl/curl.h>
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/writer.h>
#include <jsoncpp/json/value.h>
#include <sstream>
#include <fstream>
#include <unistd.h>

#define times 1 //numar de treceri

using namespace std;
using namespace rgb_matrix;

Color color(255, 255, 255);
Color color_line(255, 255, 255);
Color color_clock(255, 255, 255);
Color bg_color(0, 0, 0);

const char *bdf_font_file_text = "/home/pi/ct/fonts/9x15B.bdf";
const char *bdf_font_file_clock = "/home/pi/ct/fonts/7x13B.bdf";
const int char_width_text = 9;
const int char_width_clock = 7;
const int char_height_clock = 13;
const string def_message = "100 Pentru Viitor / RomânEști oriunde ai fi!";

int brightness = 100;
int letter_spacing = 0;
int size_of_clock;

string time_result = "";
int year = 100;
int day = 0;
int hour = 0;
int mins = 0;
int sec = 0;

double last_time_text = clock();
double last_time_clock = clock();
double last_time_timer = clock();

bool countdown = true;
bool random_set = false;

int times_left = times; //numarul de treceri

rgb_matrix::Font font_text;
rgb_matrix::Font font_clock;

char remTime[16];

static int usage(const char *progname)
{
    fprintf(stderr, "usage: %s [options]\n", progname);
    fprintf(stderr, "Reads text from stdin and displays it. "
                    "Empty string: clear screen\n");
    fprintf(stderr, "Options:\n");
    rgb_matrix::PrintMatrixFlags(stderr);
    fprintf(stderr, "\t-f <font-file>    : Use given font.\n");
    return 1;
}

// convert int to string (e bine sa stii de el)
static string itos(int i)
{
    stringstream s;
    s << i;
    return s.str();
}

//cauta sa vada daca exista deja un timp de la care sa ii dea reset
static void readTime()
{
    std::ifstream nameFileout;
    nameFileout.open("/home/pi/ct/timeFile.txt");
    std::string str;
    std::getline(nameFileout, str);
    std::vector<int> vect;
    std::stringstream ss(str);
    int i;
    while (ss >> i)
    {
        vect.push_back(i);
        if (ss.peek() == ':')
            ss.ignore();
    }
    year = vect.at(0);
    std::cout << "years: " << year << std::endl;
    day = vect.at(1);
    std::cout << "days: " << day << std::endl;
    hour = vect.at(2);
    std::cout << "hours: " << hour << std::endl;
    mins = vect.at(3);
    std::cout << "mins: " << mins << std::endl;
    sec = vect.at(4);
    std::cout << "sec: " << sec << std::endl;
}

//aici calculeaza timpul ramas in countdown timer
static string getRemainingTime()
{
    if (!countdown)
    { //cand nu face countdown, o sa afiseze "100:000:00:00:00"
        return "100:000:00:00:00";
    }

    //basic computation pentru countdown
    sec--;
    if (sec < 0)
    {
        sec = 59;
        mins--;
    }
    if (mins < 0)
    {
        mins = 59;
        hour--;
    }
    if (hour < 0)
    {
        hour = 23;
        day--;
    }
    if (day < 0)
    {
        if (year % 4 == 0)
            day = 365;
        else
            day = 364;
        year--;
    }
    if (year < 0)
    {
        countdown = false;
    }

    //formatare (pt estetica) a countdown-ului
    time_result = "";
    if (year < 100 && year > 10)
    {
        time_result = time_result + "0" + itos(year);
    }
    else if (year < 10)
    {
        time_result = time_result + "00" + itos(year);
    }
    else
        time_result = time_result + itos(year);

    time_result = time_result + ":";

    if (day < 100 && day > 10)
    {
        time_result = time_result + "0" + itos(day);
    }
    else if (day < 10)
    {
        time_result = time_result + "00" + itos(day);
    }
    else
        time_result = time_result + itos(day);

    time_result = time_result + ":";

    if (hour < 10)
    {
        time_result = time_result + "0" + itos(hour);
    }
    else
        time_result = time_result + itos(hour);

    time_result = time_result + ":";

    if (mins < 10)
    {
        time_result = time_result + "0" + itos(mins);
    }
    else
        time_result = time_result + itos(mins);

    time_result = time_result + ":";

    if (sec < 10)
    {
        time_result = time_result + "0" + itos(sec);
    }
    else
        time_result = time_result + itos(sec);

    return time_result;
}

//aici isi face load la fonturi
static int loadFonts()
{
    if (!font_text.LoadFont(bdf_font_file_text))
    {
        fprintf(stderr, "Couldn't load text font '%s'\n", bdf_font_file_text);
        return 1;
    }
    if (!font_clock.LoadFont(bdf_font_file_clock))
    {
        fprintf(stderr, "Couldn't load clock font '%s'\n", bdf_font_file_clock);
        return 1;
    }
    return 0;
}

//aici e main-ul
int main(int argc, char *argv[])
{
    cout << "Starting program " << endl;
    readTime();

    RGBMatrix::Options matrix_options;
    rgb_matrix::RuntimeOptions runtime_opt;
    if (!rgb_matrix::ParseOptionsFromFlags(&argc, &argv, &matrix_options, &runtime_opt))
    {
        return usage(argv[0]);
    }

    loadFonts();
    RGBMatrix *canvas = rgb_matrix::CreateMatrixFromOptions(matrix_options, runtime_opt);
    if (canvas == NULL)
        return 1;

    canvas->SetBrightness(brightness);

    int panel_size = 512;
    int x = panel_size;
    string remTimeString;

    char line[1024];
    strncpy(line, def_message.c_str(), sizeof(line));
    line[sizeof(line) - 1] = 0;

    int text_offset = (1024 - std::count(line, line + 1024, 0)) * char_width_text;

    while (true)
    {
        if (clock() - last_time_text > 50000)
        { //delay intre schimbare de pixeli
            last_time_text = clock();
            if (x > 0 - text_offset)
            {
                x--;
            }
            else
            {
                times_left--;
                x = panel_size;
                if (times_left <= 0)
                {
                    strncpy(line, def_message.c_str(), sizeof(line));
                    line[sizeof(line) - 1] = 0;
                    text_offset = (1024 - std::count(line, line + 1024, 0)) * char_width_text;
                    times_left = times; //numar de treceri
                }
            }
        }
        if (clock() - last_time_clock > 1301236)
        { //delay intre "secunde"
            remTimeString = getRemainingTime();
            size_of_clock = remTimeString.size();
            strncpy(remTime, remTimeString.c_str(), sizeof(remTime));
            last_time_clock = clock();
            system(("./writeTime " + time_result).c_str());
        }

        rgb_matrix::DrawText(canvas, font_text, x, 16 + font_text.baseline(),
                             color, &bg_color, line,
                             letter_spacing);

        //draw line
        rgb_matrix::DrawLine(canvas, 0, 14, panel_size, 14, color_line);
        rgb_matrix::DrawLine(canvas, 0, 15, panel_size, 15, color_line);

        //show clock
        rgb_matrix::DrawText(canvas, font_clock, 126 - (size_of_clock * char_width_clock) - 6, 12,
                             color_clock, &bg_color, remTime,
                             letter_spacing);
        rgb_matrix::DrawText(canvas, font_clock, 254 - (size_of_clock * char_width_clock) - 6, 12,
                             color_clock, &bg_color, remTime,
                             letter_spacing);
        rgb_matrix::DrawText(canvas, font_clock, 382 - (size_of_clock * char_width_clock) - 6, 12,
                             color_clock, &bg_color, remTime,
                             letter_spacing);
        rgb_matrix::DrawText(canvas, font_clock, 510 - (size_of_clock * char_width_clock) - 6, 12,
                             color_clock, &bg_color, remTime,
                             letter_spacing);
        usleep(10000);
    }
}
