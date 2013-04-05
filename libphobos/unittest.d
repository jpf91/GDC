// Written in the D programming language.

import std.getopt, core.runtime, std.stdio, std.string;

void main()
{
    
}

shared static this()
{
    Runtime.moduleUnitTester = &tester;
}

bool tester()
{
    auto args = Runtime.args;
    bool showDisabled = false;
    bool verbose = false;
    
    getopt(args,
           "show-disabled", &showDisabled,
           "verbose", &verbose);
    
    if(showDisabled)
    {
        showDisabledTests();
        return true;
    }

    bool result = true;
    
    size_t totalPassed, totalDisabled, totalFailed;

    foreach(m; ModuleInfo)
    {
        UnitTest[] moduleFailed;
        UnitTest[] moduleDisabled;
        Throwable[] moduleFailedT;
        size_t passed;
        
        if(m.unitTests.length == 0)
        {
            if(verbose)
                writefln("%s: No unit tests found", m.name);
            continue;
        }

        foreach(test; m.unitTests)
        {
            if(test.disabled)
            {
                if(verbose)
                    writefln("%-25s %10s", format("%s:%s", m.name, test.line), "DISABLED");
                moduleDisabled ~= test;
                continue;
            }
            stdout.flush();
            try
            {
                test.testFunc();
                passed++;
            }
            catch(Throwable t)
            {
                moduleFailedT ~= t;
                moduleFailed ~= test;
                writefln("%-25s %10s", format("%s:%s", m.name, test.line),  "FAILED");
            }
        }
        if(verbose)
            writefln("%s: %s passed, %s disabled, %s failed",
                m.name, passed, moduleDisabled.length, moduleFailed.length);
        
        totalPassed += passed;
        totalFailed += moduleFailed.length;
        totalDisabled += moduleDisabled.length;
        
        if(moduleFailed.length > 0)
            result = false;
    }
    writefln("Total results: Passed %s Failed %s Disabled %s", totalPassed,
        totalFailed, totalDisabled);

    return result;
}

void showDisabledTests()
{
    foreach(m; ModuleInfo)
    {
        foreach(test; m.unitTests)
        {
            if(test.disabled)
            {
                writefln("%s:%s", m.name, test.line);
            }
        }
    }
}
