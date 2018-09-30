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

static int last_id = 126;

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

int brightness = 10;
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
double last_time_db = clock();
double last_time_timer = clock();
double last_time_autosave = clock();
string last_result = "";

bool countdown = true;
bool random_set = false;

ofstream timeFile;

int times_left = times; //numarul de treceri

rgb_matrix::Font font_text;
rgb_matrix::Font font_clock;

char remTime[16];

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
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

//preia textul de la API ( http://gandeste-liber.ro/capsulatimpului/api.php?order=ASC&id=0 )
static string getApiText(int id)
{
    if (random_set)
    {
        id = 0;
    }
    try
    {
        CURL *curl;
        std::string readBuffer;
        curl = curl_easy_init();
        if (curl)
        {
            curl_easy_setopt(curl, CURLOPT_URL,
                             ("http://gandeste-liber.ro/capsulatimpului/api.php?order=ASC&id=" + itos(id)).c_str());
            //curl_easy_setopt(curl, CURLOPT_URL, ("http://192.168.0.108/capsula/api.php?order=ASC&id=" + itos(id)).c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2L);
            curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            Json::Value root;
            Json::Reader reader;
            bool parsingSuccessful = reader.parse(readBuffer.c_str(), root); //parse process
            if (!parsingSuccessful)
            {
                return def_message;
            }
            if (root[0].get("mesaj", "") != "Start Capsula-Timpului NOW")
            {
                if (random_set)
                {
                    int no = 0 + (rand() % static_cast<int>(root.size() + 1));
                    // cout << "random no is " << no << endl;
                    // cout << "random id is " << root[no].get("id", "").asString() << " and last_id is " << last_id << endl << endl;
                    return root[no].get("mesaj", "").asString();
                }
                else if (!random_set)
                {
                    last_id = atoi(root[0].get("id", last_id).asString().c_str());
                    // cout << "last_id is " << last_id << endl;
                    string result = root[0].get("mesaj", "").asString();
                    last_result = result;
                    if (result == "")
                    {
                        random_set = true;
                        return "";
                    }
                    else
                    {
                        random_set = false;
                        return result;
                    }
                }
            }
            else
            {
                return def_message;
            }
            last_id = atoi(root[0].get("id", last_id).asString().c_str());
        }
        return def_message;
    }
    catch (const std::exception &e)
    {
        if (last_result != "")
            return last_result;
        else
            return def_message;
    }
}

//aici e triggerul pt countdown
static void checkCountDownStatus()
{
    try
    {
        CURL *curl;
        std::string readBuffer;
        curl = curl_easy_init();
        if (curl)
        {
            curl_easy_setopt(curl, CURLOPT_URL, "http://gandeste-liber.ro/capsulatimpului/api.php?order=ASC");
            //curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.0.108/capsula/api.php?order=ASC");
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2L);
            curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            Json::Value root;
            Json::Reader reader;
            bool parsingSuccessful = reader.parse(readBuffer.c_str(), root); //parse process
            if (!parsingSuccessful)
            {
            }
            for (unsigned int no = 0; no < root.size(); no++)
            {
                string result = root[no].get("mesaj", "").asString();
                if (result == "Start Capsula-Timpului NOW")
                {
                    //if (result == "bbb") {
                    //if (result == "Porneste!") {
                    countdown = true;
                }
            }
        }
    }
    catch (const std::exception &e)
    {
    }
}

//aici calculeaza timpul ramas in countdown timer
static string getRemainingTime()
{
    if (!countdown)
    { //cand nu face countdown, o sa afiseze "100:000:00:00:00"
        return "100:000:00:00:00";
    }

    //basic computation pentru countdown
    if (clock() - last_time_timer > 1301236)
    {
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

        last_time_timer = clock();

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
    }
    return time_result;
}

//aici compara last_id cu ultimul id din baza de date
static void compareIdToDB()
{
    try
    {
        CURL *curl;
        std::string readBuffer;
        curl = curl_easy_init();
        if (curl)
        {
            curl_easy_setopt(curl, CURLOPT_URL,
                             ("http://gandeste-liber.ro/capsulatimpului/api.php?order=ASC&id=" +
                              itos(last_id))
                                 .c_str());
            //curl_easy_setopt(curl, CURLOPT_URL, ("http://192.168.0.108/capsula/api.php?order=ASC&id=" + itos(last_id)).c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2L);
            curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            Json::Value root;
            Json::Reader reader;
            bool parsingSuccessful = reader.parse(readBuffer.c_str(), root); //parse process
            if (!parsingSuccessful)
            {
            }
            int id = atoi(root[0].get("id", -1).asString().c_str());
            if (id > -1)
            {
                random_set = false;
                cout << "Found new entry" << endl;
            }
        }
    }
    catch (const std::exception &e)
    {
    }
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

static void getLastUsableID()
{
    try
    {
        CURL *curl;
        std::string readBuffer;
        curl = curl_easy_init();
        if (curl)
        {
            curl_easy_setopt(curl, CURLOPT_URL,
                             "http://gandeste-liber.ro/capsulatimpului/api.php?order=DESC&");
            //curl_easy_setopt(curl, CURLOPT_URL, ("http://192.168.0.108/capsula/api.php?order=ASC&id=" + itos(last_id)).c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2L);
            curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            Json::Value root;
            Json::Reader reader;
            bool parsingSuccessful = reader.parse(readBuffer.c_str(), root); //parse process
            if (!parsingSuccessful)
            {
            }
            else
            {
                int id = atoi(root[1].get("id", -1).asString().c_str());
                if (id > -1)
                {
                    last_id = id;
                    // cout << "Got the last usable id: " << last_id;
                }
            }
        }
    }
    catch (const std::exception &e)
    {
    }
}

//aici e main-ul
int main(int argc, char *argv[])
{
    cout << "Starting program " << endl;
    getLastUsableID();
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

    string temp = getApiText(last_id);
    char line[1024];
    strncpy(line, temp.c_str(), sizeof(line));
    line[sizeof(line) - 1] = 0;

    int text_offset = (1024 - std::count(line, line + 1024, 0)) * char_width_text;

    while (true)
    {
        if (clock() - last_time_autosave > CLOCKS_PER_SEC * 5)
        {
            system(("./writeTime " + time_result).c_str());
            last_time_autosave = clock();
        }
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
                    temp = getApiText(last_id);
                    strncpy(line, temp.c_str(), sizeof(line));
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
            if (!countdown)
                checkCountDownStatus();
            // cout << time_result << endl;
        }
        if (clock() - last_time_db > CLOCKS_PER_SEC && (random_set))
        { //delay intre comaparari la baza de date
            compareIdToDB();
            last_time_db = clock();
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
        usleep(100);
    }
}
