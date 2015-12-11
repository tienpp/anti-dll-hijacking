## This is simple code to prevent DLL preload attack.

To harden your Windows system, you could use 2 registry script below:
- prevent.reg: enable the hardening mode.
- reverse.reg: reverse system back to the normal stage.

I also have created the small utility for learning purpose. Please follow step to use it:
  1. Prepare your Windows driver. This could be done by compile the ProcObsrv driver in ProcMon folder.
  2. Prepare the DLL to inject into the process when it's created. Using the DLL part of ReflectiveDLLInjection.
  3. Create the main process in real time monitor the creation of the process and make the injection.

The video demo of this tool could be found here: https://www.youtube.com/watch?v=7XMxRWqFrc0

Give my best thanks to authors of 2 project ReflectiveDLLInjection and ProcMon.

"If I have seen further than others, it is by standing upon the shoulders of giants."
--Isaac Newton
