using libtta-cpp-2.3

Using:

```
#include"TtaAudioPlayer.h"
#include <iostream>
#include <conio.h>

int main(int argc, char* argv[])
{
    //// 初始化SDL音频子系统
    //if (SDL_Init(SDL_INIT_AUDIO) < 0) {
    //    std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
    //    return -1;
    //}

    Player::TtaAudioPlayer player("F:/All_Type_Audios/COCX_36010.tta");

    if (player.InitializePlayback())
    {
        player.Play();
        bool running = player.IsPlaying();
        // 等待播放结束
        while (running)
        {
            if (_kbhit())
            {
                char key = _getch();
                switch (tolower(key))
                {
                    case 'p':
                        if (player.IsPlaying())
                        {
                            player.Pause();
                            std::cout << "Paused" << std::endl;
                        }
                        else
                        {
                            player.Play();
                            std::cout << "Playing" << std::endl;
                        }
                        break;
                    case 's':
                        player.Stop();
                        std::cout << "Stopped" << std::endl;
                        break;
                    case 'f':
                        player.FastForward(5.0f);
                        std::cout << "Fast forwarded 5 seconds" << std::endl;
                        break;
                    case 'r':
                        player.Rewind(5.0f);
                        std::cout << "Rewinded 5 seconds" << std::endl;
                        break;
                    case '+':
                        player.SetVolume(std::min(1.0f, player.GetVolume() + 0.1f));
                        std::cout << "Volume: " << player.GetVolume() << std::endl;
                        break;
                    case '-':
                        player.SetVolume(std::max(0.0f, player.GetVolume() - 0.1f));
                        std::cout << "Volume: " << player.GetVolume() << std::endl;
                        break;
                    case 'q':
                        running = false;
                        break;
                    default:
                        break;
                }
            }


            {
                std::cout << "\rTime: " << player.GetCurrentTime() << " / " << player.GetTotalTime()
                    << " (" << (player.GetCurrentTime() / player.GetTotalTime() * 100) << "%)"
                    << std::flush;
            }
            SDL_Delay(100);
        }
    }

    std::cout << "---end---" << std::endl;
    system("pause");
    return 0;
}
```
