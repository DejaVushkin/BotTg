#include <tgbot/tgbot.h>
#include <iostream>
#include <set>
#include <fstream>
#include <ctime>
#include <sstream>
#include <map>
#include <regex>

using namespace std;
using namespace TgBot;

set<int64_t> userIds;
map<int64_t, bool> awaitingDate; 
set<int64_t> authorizedChatIds = { 364264732 };


string getFilenameByDate(const string& date = "") {
    if (!date.empty()) {
        return "chatid_" + date + ".txt";
    }

    time_t now = time(nullptr);
    tm localTime;
    localtime_s(&localTime, &now); 
    stringstream filename;
    filename << "chatid_" << (localTime.tm_year + 1900) << "-"
        << (localTime.tm_mon + 1) << "-"
        << localTime.tm_mday << ".txt";
    return filename.str();
}


void saveChatId(int64_t chatId) {
    string dailyFile = getFilenameByDate();
    ifstream inputFile(dailyFile);
    set<int64_t> chatIds;

   
    int64_t id;
    while (inputFile >> id) {
        chatIds.insert(id);
    }
    inputFile.close();

    if (chatIds.find(chatId) == chatIds.end()) {
        ofstream outputFile(dailyFile, ios::app);
        outputFile << chatId << endl;
        outputFile.close();

        
        userIds.insert(chatId);
    }
}


void loadChatIds(const string& date = "") {
    userIds.clear(); 
    string file = getFilenameByDate(date);
    ifstream inputFile(file);
    int64_t id;

    cout << "Loading chat IDs from file: " << file << endl;

    while (inputFile >> id) {
        userIds.insert(id);
    }
    inputFile.close();

    if (userIds.empty()) {
        cout << "No chat IDs found in file: " << file << endl;
    }
}


void sendNotifications(Bot& bot, const string& message) {
    for (const auto& chatId : userIds) {
        try {
            bot.getApi().sendMessage(chatId, message);
            this_thread::sleep_for(chrono::milliseconds(100)); 
        }
        catch (const exception& e) {
            cerr << "ERROR for chat ID " << chatId << ": " << e.what() << endl;
        }
    }
}

int main() {
    try {
        Bot bot("7311219747:AAFMQESzo38xlm6J1y-HTOrw4lc8IJ5QO_c");

        
        loadChatIds();

        bot.getEvents().onCommand("start", [&bot](Message::Ptr message) {
            saveChatId(message->chat->id); 

            string photoPath = "photo.jpg";
            string captionText = "text";

            InlineKeyboardMarkup::Ptr keyboard(new InlineKeyboardMarkup);
            vector<InlineKeyboardButton::Ptr> row;

            InlineKeyboardButton::Ptr button(new InlineKeyboardButton);
            button->text = "button";
            button->callbackData = "sendphoto2"; 

            row.push_back(button);
            keyboard->inlineKeyboard.push_back(row);

            try {
                bot.getApi().sendPhoto(
                    message->chat->id,
                    InputFile::fromFile(photoPath, "image/jpeg"),
                    captionText,
                    0,
                    keyboard
                );
            }
            catch (const TgException& e) {
                bot.getApi().sendMessage(message->chat->id, "Error");
                cerr << "Error: " << e.what() << endl;
            }
            });

        bot.getEvents().onCallbackQuery([&bot](CallbackQuery::Ptr query) {
            if (query->data == "sendphoto2") {


                string newPhotoPath = "photo2.jpg";
                string newCaptionText = "text2";

                InlineKeyboardMarkup::Ptr keyboard(new TgBot::InlineKeyboardMarkup);
                vector<InlineKeyboardButton::Ptr> row;

                InlineKeyboardButton::Ptr button(new InlineKeyboardButton);
                button->text = "Link";
                button->url = "https://example.com"; 

                row.push_back(button);
                keyboard->inlineKeyboard.push_back(row);

                try {
                    bot.getApi().sendPhoto(
                        query->message->chat->id,
                        InputFile::fromFile(newPhotoPath, "image/jpeg"),
                        newCaptionText,
                        0,
                        keyboard
                    );
                }
                catch (const TgException& e) {
                    bot.getApi().sendMessage(query->message->chat->id, "Error");
                    cerr << "Telegram API error: " << e.what() << endl;
                }
            }
            });

        bot.getEvents().onCommand("notify", [&bot](Message::Ptr message) {
            int64_t chatId = message->chat->id;

            
            if (authorizedChatIds.find(chatId) == authorizedChatIds.end()) {
                return;
            }

            awaitingDate[chatId] = true;
            bot.getApi().sendMessage(chatId, "Please send the date in the format YYYY-MM-DD to proceed.");
            });

        bot.getEvents().onAnyMessage([&bot](Message::Ptr message) {
            int64_t chatId = message->chat->id;

            
            if (awaitingDate[chatId]) {
                string date = message->text;

               
                regex datePattern("^\\d{4}-\\d{1,2}-\\d{1,2}$");
                if (!regex_match(date, datePattern)) {
                    bot.getApi().sendMessage(chatId, "Invalid date format. Please use YYYY-MM-DD.");
                    return;
                }

                try {
                    loadChatIds(date); 

                    if (userIds.empty()) {
                        bot.getApi().sendMessage(chatId, "No chat IDs found for the specified date: " + date);
                    }
                    else {
                        bot.getApi().sendMessage(chatId, "START");
                        sendNotifications(bot, "SPAM");
                        bot.getApi().sendMessage(chatId, "END");
                    }
                }
                catch (const exception& e) {
                    bot.getApi().sendMessage(chatId, "Error processing the request. Please try again.");
                    cerr << "Error: " << e.what() << endl;
                }

                awaitingDate[chatId] = false; 
            }
            });
         
        cout << "Started" << endl;
        TgLongPoll longPoll(bot);

        while (true) {
            longPoll.start();
        }
    }
    catch (const TgException& e) {
        cerr << "Error: " << e.what() << endl;
    }

    return 0;
}
