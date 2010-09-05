mc -U service_provider.mc
rc service_provider.rc

link -dll -noentry /MACHINE:X64 /out:service_provider_x64.dll service_provider.res
link -dll -noentry /MACHINE:X86 /out:service_provider_x86.dll service_provider.res