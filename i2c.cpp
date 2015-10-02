
#include <linux/i2c-dev.h>

#include <ios>
#include <iostream>

#include <boost/bind.hpp>

#include "i2c.h"

const boost::filesystem::path I2CDevice::m_defaultDeviceName{"/dev/i2c-1"};

I2CDevice::I2CDevice(const boost::filesystem::path& fileName, uint8_t slaveAddress): 
                    m_slaveAddress(slaveAddress), m_status{true}, m_timer(m_ioService), m_deviceStream(m_ioService),
                    m_source{fileName.string()};
{

    if (m_source.is_open())
    {
        m_status = false;
        std::cout << "File " << fileName.string() << " not open" << std::endl;
        return;
    }
    if (ioctl(m_source.hanle(), I2C_SLAVE_FORCE, m_slaveAddress) < 0)
    {
        m_status = false;
        std::cout << "Fail set slave address: " << static_cast<uint32_t>(m_slaveAddress) << std::endl;
        return;
    }
    boost::system::error_code ex;
    m_deviceStream.assign(m_source.hanle(), ex);
    if (ex)
    {
        std::cout << "Device stream fail: " << ex.message() << std::endl;
    }
}

void I2CDevice::StartCommunication()
{
    if (!m_status) 
    {
        std::cout << "Init device fail: " << std::endl;
        return;
    }
    StartWrite();
    StartReceive();
    m_ioService.run();
}

void I2CDevice::HandleRead(const boost::system::error_code& ex, std::size_t length)
{
    if (ex)
    {
        std::cout << "Handle read fail: " << ex.message() << std::endl;
        m_ioService.stop();
        return;  
    }
    // The actual number of bytes received is committed to the buffer so that we
    // can extract it using a std::istream object.
    m_replyBuffer.commit(length);

    // Decode the reply packet.
    std::istream is(&m_replyBuffer);
    std:: cout << "We get next message: " << std::endl;
    std::copy(std::istream_iterator<char>(is), std::istream_iterator<char>(), std::ostream_iterator<char>(std::cout));
    StartReceive();   
}

void I2CDevice::StartWrite()
{
    boost::asio::streambuf request_buffer;
    std::ostream os(&request_buffer);
    os << "Hello arm. Give me some data";
    // Send the request.
    boost::system::error_code ex;
    m_deviceStream.write_some(request_buffer.data(), ex);
    if (ex)
    {
        std::cout << "Write operation fail: " << ex.message() << std::endl;
        m_ioService.stop();
        return;
    }
    m_timeSent = boost::posix_time::microsec_clock::universal_time();
    m_timer.expires_at(m_timeSent + boost::posix_time::seconds{5});
    m_timer.async_wait(boost::bind(&I2CDevice::HandleTimeout, this, _1));
}

void I2CDevice::StartReceive()
{
    m_replyBuffer.consume(m_replyBuffer.size());
    // Wait for a reply. We prepare the buffer to receive up to 1024B.
    m_deviceStream.async_read_some(m_replyBuffer.prepare(1024),
    boost::bind(&I2CDevice::HandleRead, this, _1, _2));
}

void I2CDevice::HandleTimeout(const boost::system::error_code& ex)
{
    if (ex)
    {
        std::cout << "Handle timeout fail: " << ex.message() << std::endl;
        m_ioService.stop();
        return;   
    }
    StartWrite();
}

int main()
{
    I2CDevice device;
    device.StartCommunication();
}
