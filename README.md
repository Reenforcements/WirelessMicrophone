# Wireless Microphone (ECE 387 - Miami University)
**Austyn Larkin**

This is a wireless microphone project that was created for ECE 387 (embedded system design) at Miami University. The project was created using Arduinos, nRF24L01+ transceiver modules, LM358 operational amplifiers, and a MCP4151 digital potentiometer.
![Picture of microphone in foreground and breadboard circuits in the background.](https://github.com/Reenforcements/WirelessMicrophone/blob/master/ReadmeContent/CoverPhoto.jpg?raw=true)

# Summary

Insert video here
** A video of the wireless microphone in action. **

When the user speaks into the 600 Ohm microphone, a small signal is generated on the wires leading off the microphone. This signal's amplitude is only about 10mV, so it has to be amplified before the Arduino can read it. Before being amplified, the weak signal from the microphone is added to a 2.5V DC signal. 2.5V DC is chosen specially as it is halfway between 5V and ground. This signal is fed into an operational amplifier, where the signal is amplified from ±2.010V to roughly 5V to 0V. This signal is fed into the Arduino's ADC, where it is sampled at nearly 20kHz. Every time 16 samples are read from the ADC, they're uploaded to the nRF24L01+. After 3 packets of 16 samples have been uploaded, the nRF24L01+ sends the samples to the receiver. The receiver picks up these samples and are downloaded by the Arduino on the receiving end. The Arduino uses an internal timer to write each sample out one at a time to the digital potentiometer at a rate of about 20kHz. The digital potentiometer is used as a DAC by acting as a voltage divider. This output voltage is then shifted from 5V to 0V to ±2.5V. The voltage is then attenuated to ±0.4V so it is within the right range to be connected to speakers.

# Sending

This section describes the steps starting from the microphone input to sending out the audio as radio waves.

***

[![](https://github.com/Reenforcements/WirelessMicrophone/blob/master/ReadmeContent/SendingPhoto.jpg?raw=true)](https://github.com/Reenforcements/WirelessMicrophone/blob/master/ReadmeContent/SendingPhoto.jpg?raw=true)
**Photo 1 - A photo of the complete circuit of the sending portion of the wireless microphone.**

***

[![](https://github.com/Reenforcements/WirelessMicrophone/blob/master/ReadmeContent/Sending.png?raw=true)](https://github.com/Reenforcements/WirelessMicrophone/blob/master/ReadmeContent/Sending.png?raw=true)
**Diagram 1 - The circuit diagram for the sending portion of the wireless microphone.**

***

### Microphone Input

The first step to input the audio was to get the microphone input. This was done using a 3.5mm, 600 ohm microphone and a 3.5mm audio jack. The audio jack was connected in series with a buffered 2.5V DC source. The source was created using an equal voltage divider from the Arduino's 5V output. This way, any signal created by speaking into the microphone hovered at 2.5V instead of 0V. This is critical, as the Arduino can't read analog voltages that go negative by default.

### Transforming the Microphone Input

The signal created by the microphone (added to the 2.5V DC source) is only about ±2.010V, so before the Arduino can read it, it has to be amplified. To amplify the signal, it is fed into an LMV358IDR operational amplifier. This model of OP amp is very specific, as it is what is called a _rail to rail_ operational amplifier. This means that the operational amplifier can utilize the full 0V to 5V range that the Arduino has to offer. Without a rail to rail OP amp, we could only output a signal from 0V to 3.5V. This would introduce clipping on the top half of our audio, which would greatly degrade the final audio quality. With the rail to rail OP amp, the amplified output from the OP amp is roughly 0V to 5V.

### Sampling the Audio

The Arduino's analog to digital converter, or DAC, can read a voltage from 0V to 5V. We feed our amplified audio signal into one of the Arduino's analog inputs. We want a sampling rate of about 20,000 Hz. This introduces a problem though. The Arduino's ADC is too slow to be able to sample that quickly. The solution to this problem is to change the _ADC's prescaler_ in the Arduino's _ADCSRA_ register. The prescaler determines by how much the system's clock speed is slowed down before it is input to the ADC. It does this by diving the system clock according to the value in the prescaler. The default prescaler value is 7, which corresponds to a division of 128. Our Arduino has a 16 Mhz clock speed. This gives us an ADC clock speed of 125 kHz. An ADC conversion takes 13 ADC clock cycles. This means with the default speed we can only make about 9,600 ADC conversions a second. By changing the prescaler value to make the system clock divided by a lower number, we can get more conversions a second. This is not without consequence, however, as the faster we make the ADC run, the less precise it becomes. This is okay though, because we only want 8 bit audio and the ADC gives up to 10 bits of precision. 

Next, several interrupts are set up. The first interrupt triggers an interrupt service routine when an ADC conversion is complete. This way, we don't have to poll the ADC registers to find out when the conversion completes. The second interrupt is triggered by a timer. The timer is set up to count to 800, trigger an ADC conversion to start, and then reset. This effectively makes an interrupt that gets called at a rate of 20 kHz. This interrupt doesn't call an interrupt service routine, but instead starts a new ADC conversion when it is triggered. This way, we get evenly spaced ADC conversions that are spaced such that we get a 20 kHz sampling rate.

### Data Transmission

Within the ADC's ISR (interrupt service routine), we store the completed conversion in one space of a 16 byte array. When this array is full, it is uploaded to the nRF24L01+ transceiver through SPI. This array size is strategic, as we don't want SPI running while we're trying to do an ADC conversion. If the array size was say, 32 bytes, the upload would take too long and intersect with an ADC conversion. This would create unpleasant noise in our audio samples in the form of a high pitched beep. Once we have queued up 3 arrays of 16 bytes in the nRF24L01+, the nRF24L01+ is commanded to send the bytes out to the receiver.

# Receiving

[![](https://github.com/Reenforcements/WirelessMicrophone/blob/master/ReadmeContent/Sending.png?raw=true)](https://github.com/Reenforcements/WirelessMicrophone/blob/master/ReadmeContent/Sending.png?raw=true)
**Figure 1 - The circuit diagram for the transmitting portion of the wireless microphone.**

### Downloading Incoming Data

As the radio waves from the transmission hit the receiving nRF24L01+, they're decoded back into bytes and placed into RX FIFOs. From there, they are downloaded by the Arduino through SPI.

### Digital to Analog Conversion

The Arduino will have to output these bytes as an analog signal, but the Arduino doesn't have a DAC. The solution is to use a digital potentiometer. A _digital potentiometer_ works just like a regular potentiometer. It has three terminals, where the outer most terminals always have a constant resistance between them, and it has a wiper, which can be "moved" to vary the resistance between it and either outer pin. The major difference here is that with a digital potentiometer, we can write a byte to it through SPI to change the position of that wiper. If we use it as a voltage divider, we can get a true analog output from 0V to 5V in increments of 0.0196V.

### Outputting the Bytes

Once the Arduino has the bytes in memory, it has to write them out one at a time to the digital potentiometer. It has to try to do this at the same rate its receiving them at. To help accomplish this, a timer and accompanying interrupt service routine was set up with the same period as the timer used to sample the audio on the sending Arduino. This way, the bytes can be written out one at a time at a fairly consistent rate.

### Finishing the Signal

The digital potentiometer's 0V to 5V output needs to be adjusted a little more before the signal can be connected to speakers. First, the DC component of the signal needs to be removed. This is done by using an OP amp in a subtraction configuration and results in a ±2.5V signal. Next, the signal needs to be attenuated to be about ±0.4V. Finally, the signal is ready to be connected to speakers! 

# Video

Here's a video of the working project.
[![](https://github.com/Reenforcements/WirelessMicrophone/blob/master/ReadmeContent/VideoPreview.png?raw=true)](https://www.youtube.com/watch?v=csm7CFm7-78&feature=youtu.be)
