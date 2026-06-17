#include "main.h"

/// <summary>
/// 使用例： PlayMusic(TEXT("public/menu.mp3"));
/// </summary>
/// <param name="file"></param>
void play_music(const TCHAR* file)
{
    mciSendString(TEXT("close BGM"), NULL, 0, NULL);

    TCHAR cmd[256];

    _stprintf_s(cmd,
        TEXT("open \"%s\" alias BGM"),
        file);
    mciSendString(cmd, NULL, 0, NULL);

    // 音量归零
    mciSendString(TEXT("setaudio BGM volume to 0"), NULL, 0, NULL);

    mciSendString(TEXT("play BGM repeat"), NULL, 0, NULL);

    // Fade In
    for (int vol = 0; vol <= 500; vol += 20)
    {
        _stprintf_s(cmd,
            TEXT("setaudio BGM volume to %d"),
            vol);

        mciSendString(cmd, NULL, 0, NULL);
        Sleep(10);
    }
}

void stop_music()
{
    TCHAR cmd[64];

    // Fade Out
    for (int vol = 500; vol >= 0; vol -= 20)
    {
        _stprintf_s(cmd,
            TEXT("setaudio BGM volume to %d"),
            vol);

        mciSendString(cmd, NULL, 0, NULL);
        Sleep(10);
    }

    mciSendString(TEXT("stop BGM"), NULL, 0, NULL);
    mciSendString(TEXT("close BGM"), NULL, 0, NULL);
}