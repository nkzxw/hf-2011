The is just a showcase to decide whether EasyHook will be extended to kernel mode.

The drivers will disable PatchGuard 2 and PatchGuard 3 on all current windows versions. 

I hope you enjoy exploring this special kind of stuff.




1) Using the binaries:

Before you can use the demo application, you have to register
the "PGDisableCert.cer" in the "trusted publishers" and
"trusted root certification authorities" stores.
Additionally you need to enable the test environment:

    > bcdedit -set TESTSIGNING ON

Restart now...


2) Compiling the code

There is a post-build event which will automatically sign the
drivers with a certificate "PGDisableCert" located in
"PGDisableCertStore". It is not possible to build the drivers
with the "PGDisableCert.cer" you will find the the root directory.
You need to enable the test environment with the above command
first. Then you will have to generate your own test certificate
which will only work on your PC. You need the latest windows
driver kit to accomplish this task:

    > cd c:\winddk\bin\SelfSign
    > MakeCert -r -pe -ss PGDisableCertStore -n  "CN=PGDisableCert"  “c:\PGDisable\PGDisableCert.cer”

Now open the certificate in the Explorer by double clicking it. 
Currently the certificate is an untrusted one. To make Windows 
trusting the certificate, click “Install certificate” – “Next” – 
“place all certificates in the following store” – “Trusted Root Certification Authorities” – 
“OK” – “Next” – “Finish” and do the same procedure to add it to the “Trusted Publishers” 
store.

At this point you should be able to build the drivers assuming
that you are running on windows vista x64 and having a global
environment variable called "$(WDKROOT)" pointing to your
WDK root directory.


3) Documentation

You will find a more detailed description in the documentation
located at:

     http://www.codeplex.com/easyhook
     or http://code.google.com/p/easyhook-continuing-detours/downloads/list