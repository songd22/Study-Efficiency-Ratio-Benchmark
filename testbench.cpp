/*
SER BENCHMARK
Created by Daniel Song
Use freely, but please credit wherever possible!
Send feedback & feature suggestions to daniel.song1909@gmail.com
Happy working!
*/

#include <ncurses.h>
#include <iostream>
#include <chrono> 
#include <cmath> 
#include <fstream>
#include <iomanip>
#include <ctime>
#include <cstdlib>
#include <filesystem>
#include <string> 
using namespace std; 

const char* VERSION = "1.2"; 

int main() {

    initscr(); //PROGRAM START

    //input variables 
    char userstr[32]; 
    char userch{}; 

    //windows 
    WINDOW * interface; //window pointer called win     
    interface = newwin(5, 40, 0, 0); //initalization of a new window 
    WINDOW * data; //window displaying all statistics 
    data = newwin(10, 40, 6, 0);
    WINDOW *display; //window containing visual display
    display = newwin(12, 7, 2, 44);

    //program logic 
    double target{}; //target ser value 
    double delta = 0.0; //delta value 
    char deltaSign{}; //delta sign
    bool mode = 1; //mode 1 = active, 1 = negative 
    double ser = 100.0; //ser is initially 100

    //clock 
    using clock = std::chrono::steady_clock; 
    auto startTime = clock::now(); //time of program beginning
    auto elapsedTime{clock::duration::zero()}; //total elapsed time 
    int elapsed_s{}; //TOTAL elapsed seconds
    int seconds{}; //elapsed seconds
    int minutes{}; //elapsed minutes
    int hours{}; //elapsed hours
    auto tempBreakStart{clock::now()}; //temporary break start time
    auto tempBreakElapsed{clock::duration::zero()}; //temporary elapsed break time 
    auto totalPassive{clock::duration::zero()}; //total amount of time spent on break

    //colour configuration 
    start_color(); 
    use_default_colors();
    init_pair(2, COLOR_GREEN, -1); // positive 
    init_pair(1, COLOR_RED,   -1); //negative 

    //animation
    double flashPeriod = 0.8; //how 'strobe-y' the on and off of the last line is 
    auto lastFlash = clock::now(); //stores when the last on/off of the last line was 
    bool flashOn = 1; //stores whether or not the line is currently on or off 

    while(1) {
        nocbreak();
        echo(); 
        
        clear(); //clear and refresh procedure 
        werase(interface);
        werase(data);
        werase(display);
        refresh(); 

        mvwprintw(stdscr, 0, 0, "Welcome to SER Benchmark! (Version %s)", VERSION); 
        mvwprintw(stdscr, 1, 0, "This tool provides an overview of your session efficiency, that being the time spent working versus time elapsed."); 
        mvwprintw(stdscr, 2, 0, "-> SER: Total Session Efficiency (percentage of time spent studying versus total time elapsed)");
        mvwprintw(stdscr, 3, 0, "-> Delta: How well you are performing relative to your target session efficiency ratio"); 
    
        refresh();

        while (1) {
            mvwprintw(stdscr, 5, 0, "Select a preset TARGET SER:(Tip: What is your intention for this study session?): \n\n [1] Classic Pomodoro 83.33%% (25/5 or 50/10) \n [2] Deep Study 90%% (90/10) \n [3] Light Study 70%% \n [4] Lolligagging 25%% \n [5] Custom");
            mvwprintw(stdscr, 13, 0, "Press X to quit the program."); 
            move(14, 0); 
            userch = getch();
            if (userch == '1') {
                target = 83.333; 
                break;
            } else if (userch == '2') { 
                target = 90;
                break;
            } else if (userch == '3') {
                target = 70; 
                break;
            } else if (userch == '4') {
                target = 25;
                break;
            } else if (userch == '5') {
                while (1) {
                    clear(); 
                    mvwprintw(stdscr, 0, 0, "Please enter your custom TARGET SER % between [1, 100]: "); 
                    wgetnstr(stdscr, userstr, 31); 
                    int inputValue = atoi(userstr); 
                    if (inputValue>0 && inputValue<=100) {
                        target = inputValue;
                        break;
                    }
                }
                break; 
            } else if (userch == 'X') {
                printw("Goodbye! Press any key to exit."); 
                getch(); 
                endwin();
                return 0; 
            } 
        }
        userch = 0; //clear user input 

        clear(); 
        cbreak(); //now each input does not require enter 
        noecho(); // does not echo the user's input
        refresh(); //refreshes the whole screen
        keypad(interface, TRUE); //activate keypad
        wtimeout(interface, 20); //configuration for non-blocking input 

        wprintw(interface, "----------------------------------------"); 
        wmove(interface, 1, 6); 
        wprintw(interface, "SER BENCHMARK // TARGET %.2f%%", target); 
        wmove(interface, 2, 0); 
        wprintw(interface, "----------------------------------------"); 
        wprintw(interface, "[S] Begin Program [X] Return to Menu"); 
        wrefresh(interface); 

        while(userch!='S' && userch!='s' && userch!='X') { //input logic 
            userch = wgetch(interface); 
        }
        if (userch == 'X') {
                continue; //return to menu
        }
        userch = 0; //clear user input 

        wrefresh(display); 

        while(userch!='X' && userch!='x') {
            wclear(interface); 
            wclear(display); 
            wclear(data); 

            wprintw(interface, "----------------------------------------"); 
            wmove(interface, 1, 6); 
            wprintw(interface, "SER BENCHMARK // TARGET %.2f%%", target); 
            wmove(interface, 2, 0); 
            wprintw(interface, "----------------------------------------");
            wprintw(interface, "[T] Toggle Tracking [X] Complete Session"); 
            wrefresh(interface); 

            int input = wgetch(interface); 
            if (input!=ERR) {
                userch = char(input); 
                if (userch == 'T' || userch == 't') { 
                    if (mode==0) { //if toggled from passive to active 
                        mode = 1; 
                        totalPassive += tempBreakElapsed; //add up break time this interval to total passive time 
                        tempBreakElapsed = clock::duration::zero(); //reset tempBreakElapsed counter 
                    } else {
                        mode = 0; //if toggled from active to passive 
                        tempBreakStart = clock::now(); //record the break begin time for this interval 
                    }     
                } else if (userch == 'X') { //finish session  
                    break; 
                }
            }

            //display box - highlight red if passive green if active 
            if (mode) {
                wattron(display, COLOR_PAIR(2)); 
                box(display, 0, 0);  
                wattroff(display, COLOR_PAIR(2)); 
            } else {
                wattron(display, COLOR_PAIR(1)); 
                box(display, 0, 0); 
                wattroff(display, COLOR_PAIR(1));
            }
    
            //ELAPSED TIME CALCULATION
            elapsedTime = clock::now() - startTime; 
            elapsed_s = std::chrono::duration_cast<std::chrono::seconds>(elapsedTime).count(); //convert to seconds 
            seconds = elapsed_s; 
            minutes = seconds/60; 
            hours = minutes/60;
            minutes = minutes%60; 
            seconds = seconds%60; 

            //MODE DISPLAY 
            if (mode == 0) { 
                tempBreakElapsed = clock::now() - tempBreakStart; //calculate the break time elapsed 
                wattron(data, COLOR_PAIR(1)); 
                mvwprintw(data, 0, 0, "Mode PASSIVE"); //print in red 
                wattroff(data, COLOR_PAIR(1)); 
            } else {
                wattron(data, COLOR_PAIR(2)); 
                mvwprintw(data, 0, 0, "Mode ACTIVE "); //print in green
                wattroff(data, COLOR_PAIR(2)); 
            }
            
            //SER & DELTA CALCULATIONS
            int tempBreakElapsed_s; //convert to seconds 
            tempBreakElapsed_s = std::chrono::duration_cast<std::chrono::seconds>(tempBreakElapsed).count();
            int totalPassive_s;
            totalPassive_s = std::chrono::duration_cast<std::chrono::seconds>(totalPassive).count();

            if (elapsed_s > 0) { //if time passed is greater than 0 (division by 0 check)
                ser = (((double)elapsed_s - (double)(tempBreakElapsed_s + totalPassive_s)) / ((double)elapsed_s)); //SER = elapsed time - (total elapsed break time + current break time (if there is any)) / total elapsed time
            } else ser = 1.00; 
            delta = (((ser*100)-target)); //delta equal the difference between target ser and actual ser 

            //PRINT DATA
            mvwprintw(data, 1, 0, "%d hrs, %d mins, and %d sec", hours, minutes, seconds); //time elapsed
            mvwprintw(data, 2, 0, "SER: %.2f%%", ser*100); //ser 
            int deltaState; //negative or positive delta 
            if (delta<0) { 
                deltaSign = '-'; 
                deltaState=1; 
            } else { 
                deltaState=2; 
                deltaSign = '+';
            } 

            //COLOUR CODED DELTA DISPLAY
            wattron(data, COLOR_PAIR(deltaState)); 
            mvwprintw(data, 3, 0, "Delta: %c%.2f%%", deltaSign, fabs(delta));
            wattroff(data, COLOR_PAIR(deltaState)); 

        
            //VISUAL DISPLAY  
            int level = 10 - int(floor((ser*10))); //convert ser to scale of 10 
            if (level<1) {level = 1;} 
            for(int j=10; j>level; j--) {
                mvwprintw(display, j, 1, "-----"); 
            }
            double interflash = std::chrono::duration<double>(clock::now() - lastFlash).count(); //the time between last flash
            int colourCode = 1; 
            if (mode) colourCode = 2; 
            else colourCode = 1; 

            if (interflash>flashPeriod) { //if interval between flash is greater than preset flash period, toggle what you are drawing
                    if (flashOn) {
                        flashOn = false; 
                        mvwprintw(display, level, 1, "    "); 
                        lastFlash = clock::now();
                    } else {
                        flashOn = true; 
                        wattron(display, COLOR_PAIR(colourCode)); 
                        mvwprintw(display, level, 1, "-----"); 
                        wattroff(display, COLOR_PAIR(colourCode)); 
                        lastFlash = clock::now(); 
                    }
                } else { //if interval between flash is not greater, keep drawing what you are drawing 
                    if (flashOn) {
                        wattron(display, COLOR_PAIR(colourCode)); 
                        mvwprintw(display, level, 1, "-----"); 
                        wattroff(display, COLOR_PAIR(colourCode)); 
                    } else {
                        mvwprintw(display, level, 1, "    ");  
                    }
                } 

            wrefresh(data); 
            wrefresh(display); 
            
        }
        
        //UPON SESSION COMPLETION 

        clear(); 
        printw("SESSION CONCLUDED ------------------%%%%%%% \n"); 
        printw("Concluding SER & Delta: %.2f%% %c%.2f%%", ser*100, deltaSign, fabs(delta)); 
        printw("\nRecord in log file? [Y]");
        userch = getch(); 
        if (userch =='Y' || userch == 'y') {

            auto now_c = chrono::system_clock::to_time_t(chrono::system_clock::now()); //get current time in seconds
            int seconds = std::chrono::duration_cast<std::chrono::seconds>(elapsedTime).count(); //total elapsed time in seconds
            int minutes = seconds/60; 
            int hours = minutes/60;
            minutes = minutes%60; 
            seconds = seconds%60; 

            std::ofstream log("log.txt", std::ios::app);
            if (!log) {
                printw("Error in file append. Logs not recorded. Double check integrity of file.");
            }

            //blocking input 
            wtimeout(interface, -1);   
            echo();
            nocbreak();
            
            printw("\nPlease enter a note or title for this log: "); 
            getnstr(userstr, 31); 

            log << fixed << setprecision(2) << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M")
            << " | duration: " << hours << "hrs " << minutes << "min " << seconds << "sec"
            << " | SER & DELTA: " << ser *100 << "% " << deltaSign << fabs(delta) << " | \"" << userstr << "\"" << "\n"; 
            clear(); 
            printw("\nWrite success.\n");
            log.close(); 
        } else { 
            printw("\nAcknowledged. Logs not recorded.\n");
        }
        printw("\n\nEnter any key to return to the menu\n"); ////WORK FROM HERE 
        getch(); 
    }
}
