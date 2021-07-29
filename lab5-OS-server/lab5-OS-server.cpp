#include <Windows.h>
#include <iostream>
#include <string>
#include <conio.h>//getch

using namespace std;
const int PATH_SIZE = 80,
PASSWORD_SIZE = 4,
MAX_COUNT = 3,
EVENT_COUNT = 31;

HANDLE hEvent[EVENT_COUNT]; //создали массив событий
HANDLE hSemaphore;
LPCWSTR namesEvent[EVENT_COUNT] = { L"0",L"1",L"2",L"3",L"4", L"5", L"6", L"7",L"8",L"9",
L"!",L"@",L"#",L"$",L"%",L",",L"&",L"*",L"(",L")",L"=",L"+", L"№", L";", L"%",  L":",
L"?", L"<", L">", L"-",L"." };

LPCWSTR start = L"start";
LPCWSTR end_ = L"end";


string symbol = "0123456789!@#$%,&*()=+№;%:?<>-";

int main() {
	//wchar_t pipeName[] = L"\\\\.\\pipe\\NamedPipe";
	HANDLE hNamedPipe;
	hNamedPipe = CreateNamedPipe(TEXT("\\\\.\\pipe\\mynamedpipe"),
		PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
		PIPE_UNLIMITED_INSTANCES, 1, 1, 0, NULL);

	if (hNamedPipe == INVALID_HANDLE_VALUE)
	{
		cerr << "Error of create" << endl
			<< "Error code " << GetLastError() << endl;
		return 0;
	}

	hSemaphore = CreateSemaphore(NULL,//указатель на структуру
		MAX_COUNT, // Начальное значение счетчика
		MAX_COUNT, // Максимальное значение счетчика
		L"hSemaphore");// Имя объекта

	if (hSemaphore == NULL) {
		cerr << "Error of create semaphore" << endl
			<< "Error code " << GetLastError() << endl;
		return 0;
	}

	string password;

	cout << "If you want to exist press '-'\n";

	bool ok = true;
	do {
		ok = true;
		cout << "Create password> ";
		cin >> password;
		if (password.length() != PASSWORD_SIZE)
			ok = false;

		for (int i = 0;i < password.length();i++)
			if (password[i] < '!' || password[i] > '@')
				ok = false;
		if (!ok)
			cout << "wrong password" << endl;
	} while (!ok);


	int numberProc = 0;
	cout << "PLease enter a number of processes> \n";
	while (!(cin >> numberProc) || (cin.peek() != '\n'))
	{
		cin.clear();
		while (cin.get() != '\n');

		cerr << "Error\n";
	}

	/*обнулили поля структур перед использованием. Без этого в полях мусор который используем для запуска приложения*/
	STARTUPINFO cif;
	/*Структура STARTUPINFO используется с функциями CreateProcess, CreateProcessAsUser, CreateProcessWithLogonW,
	чтобы определить оконный терминал, рабочий стол, стандартный дескриптор и внешний вид основного окна для нового процесса.*/
	ZeroMemory(&cif, sizeof(STARTUPINFO));
	cif.cb = sizeof(STARTUPINFO);
	/*Этот идентификатор доступен до тех пор, пока поток не завершится
	и может использоваться для уникального распознавания потока внутри системы. Эти идентификаторы возвращаются в структуре PROCESS_INFORMATION.*/
	PROCESS_INFORMATION pi;

	for (int i = 0; i < numberProc; i++) {
		WCHAR path[PATH_SIZE] = L"C:\\Users\\Ksenia\\source\\repos\\lab5-OS-client\\Debug\\lab5-OS-client.exe";
		CreateProcess(path, NULL, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &cif, &pi);
		cout << "process" << i << " create\n" << endl;
	}

	//hEvent[EVENT_COUNT] = CreateEvent(NULL, TRUE, FALSE, start);
	//hEvent[EVENT_COUNT + 1] = CreateEvent(NULL, TRUE, FALSE, end_);
	for (int i = 0; i < EVENT_COUNT; i++) {
		hEvent[i] = CreateEvent(NULL,//атрибут защиты
			TRUE,//// тип сброса TRUE - ручной
			FALSE, //начальное состояние несигнальное
			namesEvent[i]);
	}


	int index = 1, clients = 0;

	while (numberProc) {
		/* Если третий аргумент равен TRUE, процедура ждет 
		перехода в сигнальное состояние всех объектов, 
		иначе — любого из указанных объектов.*/
		index = WaitForMultipleObjects(EVENT_COUNT,
			hEvent, 
			FALSE, // Задает, требуется ли ожидание всех
                                    // объектов или любого
			INFINITE);
		//cout << "ind> " << index;
		char msg[PASSWORD_SIZE];
		DWORD dwBytes;
		cout << endl;
		if (index == WAIT_FAILED)
			cout << "Wait for multiple objects failed." << endl;

		if (index < 30) {
			ReadFile(hNamedPipe, &msg, sizeof(msg), &dwBytes, NULL);

			cout << "Data from client " << (int)msg[0] << "> ";
			DisconnectNamedPipe(hNamedPipe);
			CloseHandle(hNamedPipe);
			hNamedPipe = CreateNamedPipe(TEXT("\\\\.\\pipe\\mynamedpipe"),
				PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
				PIPE_UNLIMITED_INSTANCES, MAX_COUNT, MAX_COUNT, 0, NULL);
		}

		if (index < 29)
			cout << symbol[index];

		if (index == 29) {
			ReleaseSemaphore(hSemaphore, 1, NULL);
			numberProc--;
			cout << "disconnected";
		}

		if (index == 30) {
			ReadFile(hNamedPipe, &msg, sizeof(msg), &dwBytes, NULL);
			if (msg[0] == password[0] && msg[1] == password[1] &&
				msg[2] == password[2] && msg[3] == password[3]) {
				msg[0] = (char)clients;
				cout << "client #" << clients << " connected";
				clients++;
			}
			else {
				msg[0] = '!';
				cout << "wrong password entered";
			}
			WriteFile(hNamedPipe, msg, sizeof(msg), &dwBytes, NULL);
			DisconnectNamedPipe(hNamedPipe);
			CloseHandle(hNamedPipe);
			hNamedPipe = CreateNamedPipe(TEXT("\\\\.\\pipe\\mynamedpipe"),
				PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
				PIPE_UNLIMITED_INSTANCES, MAX_COUNT, MAX_COUNT, 0, NULL);
		}
		for (int i = 0;i < EVENT_COUNT;i++)
			ResetEvent(hEvent[i]);
	}

	CloseHandle(hSemaphore);
	return 0;

}