#ifndef I2C_H_
#define I2C_H_

#include <string>

#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/asio.hpp>

class I2CDevice
{
public:
    I2CDevice(const boost::filesystem::path& fileName = m_defaultDeviceName, uint8_t slaveAddress = 0x20);
    void StartCommunication();
    void HandleRead(const boost::system::error_code& ex, std::size_t length);
    void StartWrite();   
    void StartReceive();
    void HandleTimeout(const boost::system::error_code& ex);
private:
    static const boost::filesystem::path m_defaultDeviceName;
    boost::asio::io_service m_ioService;

    bool m_status;
    uint8_t m_slaveAddress;
    boost::iostreams::file_descriptor_source m_source;
    boost::asio::posix::stream_descriptor m_deviceStream;    
    boost::asio::streambuf m_replyBuffer;
    boost::asio::deadline_timer m_timer;
    boost::posix_time::ptime m_timeSent;
};


#endif