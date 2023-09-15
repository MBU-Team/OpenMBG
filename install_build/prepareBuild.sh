#!/bin/sh
# prepareBuild.sh
# usage:
ARGS="Usage: prepareBuild <user> <SDK|demo> [skip|full]"

USER=$1
FULLGET=$3
MODE=$2

if [ "$MODE" != "demo" -a "$MODE" != "SDK" ]; then
    echo $ARGS
    exit 1
fi

cd ..

# Fetch new stuff from CVS

if [ "$FULLGET" = "skip" ]; then
    echo Skipping CVS get step
else

cvs -z9 -d :pserver:"$USER"@cvs.garagegames.com:/cvs/torque login

    if [ "$FULLGET" = "full" ]; then
        if [ "$MODE" = "demo" ]; then
            echo Getting the full example tree from CVS
            rm -rf StagingAreaCVS
            cvs -z9 -d :pserver:"$USER"@cvs.garagegames.com:/cvs/torque checkout -P -d StagingAreaCVS torque/example
        else
            echo Getting the full SDK tree from CVS
            rm -rf SDKStagingAreaCVS
            cvs -z9 -d :pserver:"$USER"@cvs.garagegames.com:/cvs/torque checkout -P -d SDKStagingAreaCVS torque
        fi
    else
        if [ "$MODE" = "demo" ]; then
            echo Updating the example tree from CVS
            cd StagingAreaCVS
            cvs -z9 -d :pserver:"$USER"@cvs.garagegames.com:/cvs/torque update -P -R -d
            cd ..
        else
            echo Updating the example tree from CVS
            cd SDKStagingAreaCVS
            cvs -z9 -d :pserver:"$USER"@cvs.garagegames.com:/cvs/torque update -P -R -d
            cd ..
        fi
    fi
fi

# Create StagingArea directory from CVS tree

if [ "$MODE" = "demo" ]; then
    rm -rf StagingArea
    cp -r StagingAreaCVS StagingArea

    find StagingArea -name "CVS" -exec rm -rf {} \;
else
    rm -rf SDKStagingArea
    cp -r SDKStagingAreaCVS SDKStagingArea

    find SDKStagingArea -name "CVS" -exec rm -rf {} \;
fi

cd install_build