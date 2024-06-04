# Hotkey-based keylogger for Windows
So, I saw a bunch of articles specifying different methods for keylogging (such as [this](https://www.elastic.co/security-labs/protecting-your-devices-from-information-theft-keylogger-protection-jp)) and I never found one that I used internally in the past.  
I thought this could be a nice opportunity to share a (not so) novel keylogging technique, based on Hotkeys!
![Hotkeyz](logo.png)

## What are hotkeys?
I decided to use the [RegisterHotKey](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-registerhotkey) WinAPI to register "hotkeys" for all across the keyboard.  
The huge advantage is that these are *system-wide* hotkeys, i.e. you get to intercept them, even before they are sent to any thread message pump.  
Basically, you supply the following arguments:
- The window - I use `NULL` since I didn't want to create my own Window. This means the registers keys are going to be associated with my thread.
- A unique identifier of type `int`.
- Modifiers (e.g. whether SHIFT is pressed). I didn't use any modifiers but you can intercept more stuff that way if you so choose.
- The [Virtual Key code](https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes) to register.

The only obstacle I had was that `RegisterHotKey` works *too well* - messages are not being sent to the intended Window!  
To solve that problem, I perform the ol' switcheroo:
- Receiving hotkeys (by intercepting `WM_HOTKEY` messages).
- Unregistering the relevant hotkey. For that I need a quick mapping between the Virtual Key code and the ID I registered the hotkey with, which is quite easy to accomplish with an array.
- Using the [keybd_event](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-keybd_event) WinAPI to simulate a keypress.
- Re-registering the hotkey again by calling [RegisterHotKey](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-registerhotkey) one more time.

While I'd never put this in "production code", the rate of typing makes it barely noticable, so this works well.

The other thing I had to take care of is not blocking - I intend on keylogging for a user-defined amount of milliseconds, so I use the [PeekMessage](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-peekmessagea) WinAPI to not block - I do poll if there are no messages to be processed. Note that I have to perform it from my own thread since the `WM_HOTKEY` Window Messages are going to only arrive to my thread message queue, as stated in MSDN.

## The code
I ended up coding a single file `Hotkeyz.c`, but I uploaded the Visual Studio 2019 project here.  
You can easily re-implement in PowerShell or compile it as a DLL instead of an executable. However, I bear no responsibility on how this is going to be used.

Stay tuned,

Jonathan Bar Or
