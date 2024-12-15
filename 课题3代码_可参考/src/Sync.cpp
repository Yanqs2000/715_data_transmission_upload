#include "Sync.h"
#include "gloghelper.h"

struct tm get_tm(std::vector<std::string>& date_time)
{
    struct tm timer;
    if(date_time.size() < 2)
    {
        //std::cout<<"get_tm: date_time size < 2"<<std::endl;
        timer.tm_year = -1;
        return timer;
    }
    std::string date_str = date_time[0];
    std::string time_str = date_time[1];
    std::string yearStr;
    std::string monthStr;
    std::string dayStr ; 
    std::string hourStr;
    std::string minuteStr;
    std::string secondStr;
    if(date_str.size() != 6 || time_str.size() != 6)
    {
       // std::cout<<"date_str or time_str size != 6"<<std::endl;
        timer.tm_year = -1;
        return timer;
    }
    try
    {
        /* code */
        yearStr = date_str.substr(0, 2);
        monthStr = date_str.substr(2, 2);
        dayStr = date_str.substr(4, 2);

        hourStr = time_str.substr(0, 2);
        minuteStr = time_str.substr(2, 2);
        secondStr = time_str.substr(4, 2);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n' <<std::endl;
        timer.tm_year = -1;
        return timer;
    }

    int year = std::stoi(yearStr) + 2000;
    if(year > 1900 && year < 2100)
    {
        timer.tm_year = year - 1900;
    }else{
        timer.tm_year = -1;
    }
    int month = std::stoi(monthStr) - 1;
    if(month > 0 && month < 12)
    {
        timer.tm_mon = month;
    }else{
        timer.tm_mon = -1;
    }
    int day = std::stoi(dayStr);
    if(day > 0 && day < 31)
    {
        timer.tm_mday = day;
    }else{

        timer.tm_mday = -1;
    }
    int hour = std::stoi(hourStr);
    if(hour > 0 && hour < 24)
    {
        timer.tm_hour = hour;
    }else{

        timer.tm_hour = -1;
    }
    int minute = std::stoi(minuteStr);
    if(minute > 0 && minute < 60)
    {
        timer.tm_min = minute;
    }else{

        timer.tm_min = -1;
    }
    int second = std::stoi(secondStr);
    if(second > 0 && second < 60)
    {
        timer.tm_sec = second;
    }else{

        timer.tm_sec = -1;
    }
    return timer;
}



int sync_run_raw(std::vector<std::uint32_t>& date_time)
{
    struct tm timer;
    if(date_time.size() < 2)
    {
       // std::cout<<"date_time size < 2"<<std::endl;
        return -1;
    }
    std::uint32_t date = date_time[0];
    std::uint32_t time = date_time[1];
    char dateTime_str[20];
    try
    {
        /* code */
        int year = date / 10000;
        int temp_date = date % 10000;
        int month = temp_date / 100;
        int day = temp_date % 100;
        // 提取小时、分钟、秒
        int hour = time / 10000;
        int temp_time = time % 10000;
        int minute = temp_time / 100;
        int second = temp_time % 100;

        sprintf(dateTime_str, "%d-%02d-%02d %02d:%02d:%02d", year, month,day,hour, minute, second);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n' <<std::endl;
        return -1;
    }
	/*
    // std::string cmd = "sudo date -s \"" + dateTime+"\" > /dev/null 2>&1";
    std::string dateTime = dateTime_str;
    std::string cmd = "sudo date -s \"" + dateTime+"\"";
    int res = system(cmd.c_str());

    if (WIFEXITED(res) && WEXITSTATUS(res) == 0) {
        // std::cout << "Command executed successfully." << std::endl;
        // return 1;
    } else {
        return -1;
       // std::cerr << "Command execution failed with status: " << WEXITSTATUS(res) << std::endl;
       LOG(ERROR) << "Conmand execution failed with status: " << WEXITSTATUS(res) <<std::endl;
    }
    // std::this_thread::sleep_for(std::chrono::seconds(1));
    */
    return 1;
}

int get_inctimestr_raw(std::vector<std::uint32_t>& date_time, std::string& timestr)
{
    if(date_time.size() < 2)
    {
        // std::cout<<"get_inctimestr: date_time size < 2"<<std::endl;
        LOG(ERROR)<<"get_inctimestr: date_time size < 2" ;
        return -1;
    }
    uint32_t date = date_time[0];
    uint32_t time = date_time[1];
    char datetime_char[20];
    
    try
    {
        /* code */
        sprintf(datetime_char, "%06d-%06d", date, time);
        timestr = datetime_char;
    }
    catch(const std::exception& e)
    {
        // std::cerr << e.what() << '\n'<<std::endl;
        LOG(ERROR)<<"get_inctimestr: "<<e.what() << '\n' ;
        return -1;
    }
    return 1;
}
