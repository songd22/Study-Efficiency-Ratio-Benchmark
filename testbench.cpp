#include <ncurses.h>
#include <iostream>
#include <chrono> 
#include <cmath> 
#include <fstream>
#include <iomanip>
#include <ctime>
#include <cstdlib>
#include <filesystem>
using namespace std; 

const string VERSION = "1.0"; 

int main() {

    //configration mode: 
    cout << "\033[2J\033[1;1H" << flush;
    cout << "\n\n Welcome to SER Benchmark! (Version " << VERSION << ")" << endl; 
    cout << "\nThis tool provides an overview of your session efficiency, that being the time spent working versus the time spent taking breaks: " <<endl; 
    cout << " \n-> SER: Total session efficiency (percentage of time spent studying versus total time elapsed) \n -> Delta: How well you are performing versus your recent session efficiency \n \n Created by Daniel Song" << endl;

    double usernum{}; //user input 
    while (1) {
        cout << "\n\nPlease enter a target SER (%): \n Tips/Presets: \n -> Classic Pomodoro 50/10 or 25/5: 83.33% SER \n -> Deep Study 90/10: 90.00% SER \n -> Light Study 70/30: 70.00% SER" << endl; 
        cin >> usernum; 
        if (usernum<=0 || usernum > 100) {
            cout << "Please input a value between {0, 100}." << endl; 
        } else {
            break; 
        }
    }
    
    //ncurses initialization

    initscr(); //NCURSES BEGIN
    cbreak(); //now each input does not require enter 
    noecho(); // does not echo the user's input
    refresh(); //refreshes the whole screen
    
    WINDOW * interface; //window pointer called win     
    interface = newwin(5, 40, 0, 0); //initalization of a new window 
    double delta = 0.0; 
    char deltaSign{}; 

    keypad(interface, TRUE); //activate keypad

    wprintw(interface, "----------------------------------------"); 
    wmove(interface, 1, 6); 
    wprintw(interface, "SER BENCHMARK // TARGET %.2f%%", usernum); 
    wmove(interface, 2, 0); 
    wprintw(interface, "----------------------------------------"); 
    wprintw(interface, "[S] Begin Program [X] Quit"); 
    wrefresh(interface); 

    char userc{}; //initial user input
    while(userc!='S' && userc!='s') {
        userc = wgetch(interface); 
        if (userc == 'X') {
            clear();
            printw("Goodbye! Press any key to exit."); 
            getch(); 
            return 1;
        }
    }

    wtimeout(interface, 20); //configuration for non-blocking input 

    /////////////////////////// PROGRAM LOGIC

    WINDOW * data; //window displaying all statistics 
    data = newwin(10, 40, 6, 0);

    WINDOW *display; //window containing visual display
    display = newwin(12, 7, 2, 44);
    wrefresh(display); 

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
    double target = usernum; //target SER value 

    //colour configuration 
    start_color(); 
    use_default_colors();
    init_pair(2, COLOR_GREEN, -1); // positive 
    init_pair(1, COLOR_RED,   -1); //negative 
    
    userc = 0; 
    bool mode = 1; //mode 1 = active, 1 = negative 
    double ser = 100.0; //ser is initially 100

    //animation
    double flashPeriod = 0.8; //how 'strobe-y' the on and off of the last line is 
    auto lastFlash = clock::now(); //stores when the last on/off of the last line was 
    bool flashOn = 1; //stores whether or not the line is currently on or off 

    while(userc!='X' && userc!='x') {
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
            userc = char(input); 
            if (userc == 'T' || userc == 't') { 
                if (mode==0) { //if toggled from passive to active 
                    mode = 1; 
                    totalPassive += tempBreakElapsed; //add up break time this interval to total passive time 
                    tempBreakElapsed = clock::duration::zero(); //reset tempBreakElapsed counter 
                } else {
                    mode = 0; //if toggled from active to passive 
                    tempBreakStart = clock::now(); //record the break begin time for this interval 
                }     
            } else if (userc == 'X') { //quit 
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
   
        //calculate elapsed time 
        elapsedTime = clock::now() - startTime; 
        elapsed_s = std::chrono::duration_cast<std::chrono::seconds>(elapsedTime).count(); //convert to seconds 
        seconds = elapsed_s; 
        minutes = seconds/60; 
        hours = minutes/60;
        minutes = minutes%60; 
        seconds = seconds%60; 

        if (mode == 0) { //if passive mode 
            tempBreakElapsed = clock::now() - tempBreakStart; //calculate the break time elapsed 
            wattron(data, COLOR_PAIR(1)); 
            mvwprintw(data, 0, 0, "Mode PASSIVE"); //print in red 
            wattroff(data, COLOR_PAIR(1)); 
        } else {
            wattron(data, COLOR_PAIR(2)); 
            mvwprintw(data, 0, 0, "Mode ACTIVE "); //print in green
            wattroff(data, COLOR_PAIR(2)); 
        }
           
        int tempBreakElapsed_s; //convert to seconds 
        tempBreakElapsed_s = std::chrono::duration_cast<std::chrono::seconds>(tempBreakElapsed).count();
        int totalPassive_s;
        totalPassive_s = std::chrono::duration_cast<std::chrono::seconds>(totalPassive).count();

        if (elapsed_s > 0) { //if time passed is greater than 0 (division by 0 check)
            ser = (((double)elapsed_s - (double)(tempBreakElapsed_s + totalPassive_s)) / ((double)elapsed_s)); //SER = elapsed time - (total elapsed break time + current break time (if there is any)) / total elapsed time
        } else ser = 1.00; 
        
        //delta calculations
        delta = (((ser*100)-target)); //delta equal the difference between target ser and actual ser 

        //print data 
        mvwprintw(data, 1, 0, "%d hrs, %d mins, and %d sec", hours, minutes, seconds);
        mvwprintw(data, 2, 0, "SER: %.2f%%", ser*100);
        int deltaState; 
        if (delta<0) { 
            deltaSign = '-'; 
            deltaState=1; 
        } else { 
            deltaState=2; 
            deltaSign = '+';
        } 
        wattron(data, COLOR_PAIR(deltaState)); 
        mvwprintw(data, 3, 0, "Delta: %c%.2f%%", deltaSign, fabs(delta));
        wattroff(data, COLOR_PAIR(deltaState)); 

    
        //display 
        
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
    
    clear(); 
    printw("SESSION CONCLUDED ------------------%%%%%%% \n"); 
    printw("Concluding SER & Delta: %.2f%% %c%.2f%%", ser*100, deltaSign, fabs(delta)); 
    printw("\nRecord in log file? [Y]");
    userc = getch(); 
    if (userc =='Y' || userc == 'y') {

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
        char* userstr = new char[50]; 
        getstr(userstr); 

        log << fixed << setprecision(2) << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M")
        << " | duration: " << hours << "hrs " << minutes << "min " << seconds << "sec"
        << " | SER: " << ser *100 << "% | \"" << userstr << "\""; 
        clear(); 
        printw("\nWrite success.\n");
        log.close(); 
    } else printw("\nAcknowledged. Logs not recorded.\n"); 

    printw("Goodbye! Press any key to exit."); 
    getch(); 
    endwin();
    return 0; 
}

