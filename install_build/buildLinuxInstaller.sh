# build the loki setup installer
CWD=`pwd`
INSTALLER_TOP=$CWD/gginstall
INSTALLER_DEST=LokiSetup
STAGING=StagingArea
        
if [ ! -d $INSTALLER_TOP ]; then
    echo "Installer dir does not exist: $INSTALLER_TOP"
    echo ""
    echo "You can download GGInstall from the GarageGames site.  If it is"
    echo "not available then email johnw@lowestplane.org.  Install or symlink"
    echo "it to $INSTALLER_TOP."
    exit 1
fi

cd ..

# smoke the old one
rm -rf $INSTALLER_DEST
# copy new one
cp -a $INSTALLER_TOP/setup/$INSTALLER_DEST .

# nuke cvs files
rm -rf `find $INSTALLER_DEST -iname "CVS" -type d`
# smoke emacs backups files
rm -f `find $INSTALLER_DEST -iname "*~" -type f`

# copy license into staging and setup dir
fold -s -w 70 install_build/licenseDemo.txt > $STAGING/license.txt
cp -a $STAGING/license.txt $INSTALLER_DEST

# tar 
cd $STAGING
tar -czf ../$INSTALLER_DEST/TarBall.tar.gz *
cd ..

# copy launcher
cp -a $STAGING/demo.sh $INSTALLER_DEST/bin/Linux/x86

# copy installer stuff
cp -a install_build/linux_installer/icon.xpm $INSTALLER_DEST
#cp -a install_build/linux_installer/splash.xpm $INSTALLER_DEST/setup.data
cp -a install_build/linux_installer/setup.xml $INSTALLER_DEST/setup.data

# make self extracting file
$INSTALLER_TOP/makeself/makeself.sh $INSTALLER_DEST install_build/TorqueDemo.sh.bin "Torque Demo" ./setup.sh 


