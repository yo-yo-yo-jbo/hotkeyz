# Hotkey-based keylogger for Windows
So, I saw a bunch of articles specifying different methods for keylogging (such as [this](https://www.elastic.co/security-labs/protecting-your-devices-from-information-theft-keylogger-protection-jp)) and I never found one that I used internally in the past.  
I thought this could be a nice opportunity to share a (not so) novel keylogging technique, based on Hotkeys!

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
