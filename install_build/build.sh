#!/bin/sh

PLAT=$1
TARGET=$2

ARGS="Usage: build [win32|macosx|linux] [SDK|demo]"

if [ "$PLAT" != "win32" -a "$PLAT" != "macosx" -a "$PLAT" != "linux" ]; then
    echo $ARGS
    exit 1
fi

if [ "$TARGET" != "SDK" -a "$TARGET" != "demo" ]; then
    echo $ARGS
    exit 1
fi

if [ "$TARGET" = "SDK" ]; then
    cd ../SDKStagingArea/example
    cp ../../example/torqueDemo.exe .
    ../../install_build/upx.exe -9 torqueDemo.exe
	cp ../../example/glu2d3d.dll .
	cp ../../example/opengl2d3d.dll .
    cd ../../install_build
    makensis-bz2 Torque.nsi
else

    cd ../StagingArea
    rm -rf starter.fps show tutorial.base starter.racing
    sed -e 's/starter.fps/demo/' < main.cs > main.cs.new
    mv main.cs.new main.cs

    # Convert pngs in the demo tree into jpgs
    # But... first move all the bitmap arrays out of the way.

    mv demo/client/ui/demoCheck.png ./demoCheck.pno
    mv demo/client/ui/demoMenu.png ./demoMenu.pno
    mv demo/client/ui/demoRadio.png ./demoRadio.pno
    mv demo/client/ui/demoScroll.png ./demoScroll.pno
    mv demo/client/ui/demoWindow.png ./demoWindow.pno
    mv demo/client/ui/hudfill.png ./hudfill.pno
    mv demo/client/ui/chatHudBorderArray.png ./chatHudBorderArray.pno

    mv -f main.cs main.real
    cp ../install_build/png2jpg.cs main.cs

    if [ "$PLAT" = "win32" ]; then
	    cp ../example/torqueDemo.exe demo.exe
        NAMEARG="-name"
	    ./demo.exe
    fi
    if [ "$PLAT" = "macosx" ]; then
        NAMEARG="-name"
	    cp -r "../example/Torque Demo OSX.app" .
	    "./Torque Demo OSX.app/Contents/MacOS/Torque Demo OSX"
    fi
    if [ "$PLAT" = "linux" ]; then
        NAMEARG="-iname"
            # smoke previous binaries & libs
            rm -rf demo.bin 
            rm -rf lib

            # copy the binary
            cp -a ../example/torqueDemo.bin demo.bin
            # strip binary
            strip -p demo.bin
            # copy third party libs
            cp -a ../example/lib .
            # copy third party legalese
            cp -a `find ../tools/unix/tplib_template/ -type f | grep -v CVS` ../example/lib
            # copy the demo launcher 
            cp ../tools/unix/demo_launcher.sh demo.sh
            # make it executable
            chmod +x demo.sh
            # run it. use -nohomedir so output files go here
            ./demo.sh -nohomedir
    fi

    mv -f main.real main.cs
    find demo "$NAMEARG" "*.png" -exec rm -f {} \;

    mv ./demoCheck.pno demo/client/ui/demoCheck.png 
    mv ./demoMenu.pno demo/client/ui/demoMenu.png 
    mv ./demoRadio.pno demo/client/ui/demoRadio.png 
    mv ./demoScroll.pno demo/client/ui/demoScroll.png 
    mv ./demoWindow.pno demo/client/ui/demoWindow.png 
    mv ./hudfill.pno demo/client/ui/hudfill.png 
    mv ./chatHudBorderArray.pno demo/client/ui/chatHudBorderArray.png 

    if [ "$PLAT" = "win32" ]; then
	    cp ../example/glu2d3d.dll .
	    cp ../example/opengl2d3d.dll .
	    cp ../example/OpenAL32.dll .
    else
	    rm -f OpenAL32.dll
    fi

    if [ ! "$PLAT" = "linux" ]; then
            rm -f runtorque.sh
    fi

    mv -f demo/ReadMe.html .

    # Compress exe
    if [ "$PLAT" = "win32" ]; then
	    chmod +w demo.exe
	    ../install_build/upx.exe -9 demo.exe
    fi
    # Cleanup
    cd ..
    find StagingArea "$NAMEARG" "*.psd" -exec rm -f {} \;
    find StagingArea "$NAMEARG" "*.max" -exec rm -f {} \;
    find StagingArea "$NAMEARG" "*.qrk" -exec rm -f {} \;
    find StagingArea "$NAMEARG" "*.dso" -exec rm -f {} \;
    find StagingArea "$NAMEARG" "*.map" -exec rm -f {} \;
    find StagingArea "$NAMEARG" "*.cfg" -exec rm -f {} \;
    find StagingArea "$NAMEARG" "*.log" -exec rm -f {} \;
    find StagingArea "$NAMEARG" "*.wad" -exec rm -f {} \;
    find StagingArea "$NAMEARG" "CVS" -exec rm -rf {} \;
    find StagingArea "$NAMEARG" "\.#*" -exec rm -rf {} \;
    find StagingArea "$NAMEARG" "*.ml" -exec rm {} \;

    # Nothing writable, except the .ml file
    #chmod -R -w *
    #chmod +w demo/data/missions/*.ml
    cd StagingArea
    chmod -R +w *

    # build the installer for it...
    cd ../install_build
    if [ "$PLAT" = "win32" ]; then
	    makensis-bz2 TorqueDemo.nsi
    fi
    if [ "$PLAT" = "macosx" ]; then 
	    rm TorqueDemo.dmg
	    rm TorqueDemoPrep.dmg
	    hdiutil create -megabytes 30 -ov -fs HFS+ -volname "TorqueDemo" ./TorqueDemoPrep.dmg -srcfolder ../StagingArea
#	    hdid ./TorqueDemoPrep.dmg
#	    ditto -rsrcFork ../StagingArea "/Volumes/TorqueDemo"
#	    hdiutil detach disk1s2
	    hdiutil convert -format UDZO -imagekey zlib-level=9 -o TorqueDemo.dmg TorqueDemoPrep.dmg
	    hdiutil internet-enable ./TorqueDemo.dmg
    fi
    if [ "$PLAT" = "linux" ]; then
            sh buildLinuxInstaller.sh
    fi
fi

