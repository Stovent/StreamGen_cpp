java -version
SET location=%~dp0
REM ava -Xmx11500m -Xms11000m -cp %location%\dist\Coron4MagaliceO.jar;%location%\dist\lib\* fr.loria.coronsys.coron.Main C:/Users/_tmtrge/Documents/NetBeansProjects/CICLAD/testdb_gen2.rcf  2 -alg:touch1 > touch_testdb_gen2_supp_2.txt
java -Xmx11500m -Xms4000m -cp %location%\dist\Coron4MagaliceO.jar;%location%\dist\lib\* fr.loria.coronsys.coron.Main C:/Users/_tmtrge/Documents/NetBeansProjects/CICLAD/retail_reduced.rcf  0 -alg:touch1 > touch_retail_reduced_all.txt
REM java -Xmx11500m -Xms4000m -cp %location%\dist\Coron4MagaliceO.jar;%location%\dist\lib\* fr.loria.coronsys.coron.Main C:/Users/_tmtrge/Documents/NetBeansProjects/CICLAD/retail_reduced_80_100.rcf  0 -alg:touch1 > touch_retail_reduced_80_100.txt