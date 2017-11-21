# Keppi_2017
Repository of Bela files for Keppi, a musical instrument

This project was made with Bela (bela.io). To try it out, load the `Keppi/` folder onto your Bela board.

## A note on testing resources

Keppi is a complex instrument with a number of sensors and components (capacitive touch over i2c, accelerometer, piezo networks, and LED lights). 

I found in development that bugs were very difficult to find, because of the complex confluence of these multiple components. The `Testing library/` folder is a group of files that are designed to make deugging easy, by allowing you to isolate and test various aspects.
