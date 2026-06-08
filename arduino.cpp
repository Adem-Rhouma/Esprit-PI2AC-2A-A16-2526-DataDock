#include "arduino.h"

Arduino::Arduino()
{
    data="";
    arduino_port_name="";
    arduino_is_available=false;
    serial=new QSerialPort;
}


Arduino::~Arduino() {
    close_arduino();
    delete serial;
}



QString Arduino::getarduino_port_name()
{
    return arduino_port_name;
}

QSerialPort *Arduino::getserial()
{
   return serial;
}
int Arduino::connect_arduino()

{
    foreach (const QSerialPortInfo &serial_port_info, QSerialPortInfo::availablePorts()){
        bool is_arduino = false;
        if(serial_port_info.hasVendorIdentifier() && serial_port_info.hasProductIdentifier()){
            if((serial_port_info.vendorIdentifier() == arduino_uno_vendor_id && serial_port_info.productIdentifier() == arduino_uno_producy_id) ||
               (serial_port_info.vendorIdentifier() == 0x2341) || // Original Arduino VID
               (serial_port_info.description().contains("Arduino", Qt::CaseInsensitive)))
            {
                is_arduino = true;
            }
        }
        
        if (is_arduino) {
            arduino_is_available = true;
            arduino_port_name = serial_port_info.portName();
            break;
        }
    }

    qDebug() << "arduino_port_name is :" << arduino_port_name;

#ifdef Q_OS_LINUX
    // FALLBACK: on Linux the VID/PID may not be reported by the kernel driver.
    // Try common ACM/USB ports if nothing was found above.
    if (!arduino_is_available) {
        const QStringList linuxFallbacks = { "ttyACM0", "ttyACM1", "ttyUSB0", "ttyUSB1" };
        for (const QString &portName : linuxFallbacks) {
            const QString fullPath = "/dev/" + portName;
            if (QFile::exists(fullPath)) {
                arduino_is_available = true;
                arduino_port_name = portName;
                qDebug() << "Arduino: Linux fallback port found:" << portName;
                break;
            }
        }
    }
#endif
    if(arduino_is_available){ // si la carte est trouvée
        serial->setPortName(arduino_port_name);
        if(serial->open(QIODevice::ReadWrite)){
            serial->setBaudRate(QSerialPort::Baud9600); // débit : 9600 bits/s
            serial->setDataBits(QSerialPort::Data8);   // longueur des données : 8 bits,
            serial->setParity(QSerialPort::NoParity);    // no parité
            serial->setStopBits(QSerialPort::OneStop);    // nombre de bits de stop : 1
            serial->setFlowControl(QSerialPort::NoFlowControl);
            return 0;
        }
        return 1;
    }
    return -1;

}

int Arduino::close_arduino()

{

    if(serial->isOpen()){
            serial->close();
            return 0;
        }
    return 1;


}



QByteArray Arduino::read_from_arduino()
{
   if(serial->isReadable()){
        data=serial->readAll(); //récupérer les données reçues

        return data;
   }
   return "";
 }


int Arduino::write_to_arduino( QByteArray d)


{

    if(serial->isWritable()){

        serial->write(d);  // envoyer des donnés vers l'arduino
        return 0;
    }else{
        qDebug() << "Couldn't write to serial!";
        return 1;

    }


}
