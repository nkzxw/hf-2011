#pragma once

#include <string>

using std::string;

bool DialogASK(const string& title, string& input);
bool DialogMSG(const string& content, int& input);
bool DialogMSGYN(const string& content, int& input);

void MsgInfo(const string& sMsg);
void MsgWarning(const string& sMsg);
void MsgError(const string& sMsg);
