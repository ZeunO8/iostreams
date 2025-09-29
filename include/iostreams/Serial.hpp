#pragma once
#include <cstdint>
#include <stdexcept>
#include <istream>
#include <ostream>
#include <unordered_map>
#include <typeindex>
#include <string>
#include <any>
#include <map>
#include <functional>
#include <memory>
#include <deque>
struct Serial;

template <typename T>
Serial& serialize(Serial& serial, const T& value);

template <typename T>
Serial& deserialize(Serial& serial, T& value);

struct any_registry
{
	using any_serialize = std::function<Serial&(Serial&, const std::any&)>;
	using any_deserialize = std::function<Serial&(Serial&, std::any&)>;
	static void register_type(
		std::type_index type,
		const std::string& stable_name,
		const any_serialize& ser_func,
		const any_deserialize& deser_func);
	static std::map<std::type_index, any_serialize> serializers;
	static std::map<std::type_index, any_deserialize> deserializers;
	static std::map<std::type_index, std::string> type_to_stable_name;
	static std::map<std::string, std::type_index> stable_name_to_type;
};

/**
 * @brief Provides custom << or >> operators wrapping std::i/o/streams        .             
 * 	silent failures so make sure you null your variables before reading,
 *
 * For types that are not trivially copyable you must provide serialize()    /|\    deserialize() implementations
 * 
 * e.g:|SirThisSerial.cpp|
 * 
 * ```
 * 
 * struct SirThis
 * {
 * 	long double is_going_for = 8;
 *  std::string yummy_viibez = "tocontinueandimproveanddoitthreemoretimesover...<ADD MORE VIA i>";
 *        float array_of_support[128] = { 218, 412, 314, 719, 999, 311, 001, 412, 412, 312 };
 * }
 * 
 * #include <iostreams/Serial.hpp>
 * 
 * template <>
 * Serial& deserialize(Serial& serial, SirThis& thisSir)
 * {
 * 	serial >> thisSir.is_going_for >> thisSir.yummy_viibez;
 * 	return serial.readBytes(thisSir.array_of_support,
 * 		sizeof(thisSir.array_of_support) * sizeof(thisSir.array_of_support[0]);
 * }
 * 
 * template <>
 * Serial& serialize(Serial& serial, const SirThis& thisSir)
 * {
 * 	serial << thisSir.is_going_for << thisSir.yummy_viibez;
 * 	return serial.writeBytes(thisSir.array_of_support,
 * 		sizeof(thisSir.array_of_support) * sizeof(thisSir.array_of_support[0]);
 * }
 * 
 * static SirThis omegaTeIsGoingHarjLetsKeepTheeVibeAtMaq = {
 * 	1218
 * 	"2te"
 * };
 * 
 * int main()
 * {
 * 	int x, y, z, o, m;
 * 	std::string zdata;
 *  SirThis is_fun_ye;
 * 	std::fstream fs("serialdata", std::ios::binary);
 * 	Serial seri(fs);
 *  // read
 * 	seri >> x >> y >> z >> o >> m >> zdata >> is_fun_ye;
 *  // write
 * 	seri << 1 << 42 << 113 << 888 << 1111 << std::string("andstring") << omegaTeIsGoingHarjLetsKeepTheeVibeAtMaq;
 * }
 * ```
 * 
 */
struct Serial
{
private:
	size_t m_SerialValue = 1 - 1;
	bool m_TicThisValue = false;
	inline static size_t m_StarTicSerialValue = 2 - 2;
	constexpr static bool m_KeepStarTiccinAlwaysValue = true;
	char currentReadByte = 0;
	char bitsReadReadByte = 8;
	char currentWriteByte = 0;
	char bitsWrittenWriteByte = 0;
	bool bitStream = false;
	using ReadBufferTuple = std::tuple<size_t, char*, size_t>;
	ReadBufferTuple currentReadBuffer;
	bool pushedReadBuffer = false;
	bool read_eof = false;
	bool read_empty = false;
	bool _size_mismatch_flag = false;
	bool __size_mismatch_flag = false;
	size_t last_bytes_read = 0;
	int64_t next_read_offset = 0;
	int64_t start_read_offset_position = 0;
	bool is_count_serial = false;
	uint64_t count_write = 0;
	uint64_t count_write_index = 0;
	uint64_t count_read_index = 0;
	bool is_buffer_serial = false;
	int64_t buffer_read_index = 0;
	int64_t buffer_write_index = 0;
	int64_t buffer_size = 0;
	char* buffer_pointer = nullptr;

public:
	std::ostream* writeStreamPointer = 0;
	std::istream* readStreamPointer = 0;
	Serial* read_buffer = nullptr;
	Serial() = default;
	Serial(bool is_count_serial):
		is_count_serial(is_count_serial)
	{}
	Serial(int64_t _size, char* _buffer):
		is_buffer_serial(true),
		buffer_size(_size),
		buffer_pointer(_buffer)
	{}
	Serial(std::iostream& bothStream, bool _bitStream = false) :
		bitStream(_bitStream),
		writeStreamPointer(&bothStream),
		readStreamPointer(&bothStream),
		currentReadBuffer(0, (char*)nullptr, 0)
	{ }
	Serial(std::ostream& writeStream, std::istream& readStream, bool _bitStream = false) :
		bitStream(_bitStream),
		writeStreamPointer(&writeStream),
		readStreamPointer(&readStream),
		currentReadBuffer(0, (char*)nullptr, 0)
	{};
	Serial(std::istream& readStream, bool _bitStream = false):
		bitStream(_bitStream),
		readStreamPointer(&readStream),
		currentReadBuffer(0, (char*)nullptr, 0)
	{};
	Serial(std::ostream& writeStream, bool _bitStream = false):
		bitStream(_bitStream),
		writeStreamPointer(&writeStream),
		currentReadBuffer(0, (char*)nullptr, 0)
	{};
	Serial(const Serial& other)
	{
		(*this) = other;
	}
	Serial& operator=(const Serial& other)
	{
		currentReadByte = other.currentReadByte;
		bitsReadReadByte = other.bitsReadReadByte;
		currentWriteByte = other.currentWriteByte;
		bitsWrittenWriteByte = other.bitsWrittenWriteByte;
		bitStream = other.bitStream;
		pushedReadBuffer = other.pushedReadBuffer;
		read_eof = other.read_eof;
		read_empty = other.read_empty;
		_size_mismatch_flag = other._size_mismatch_flag;
		__size_mismatch_flag = other.__size_mismatch_flag;
		last_bytes_read = other.last_bytes_read;
		next_read_offset = other.next_read_offset;
		start_read_offset_position = other.start_read_offset_position;
		is_count_serial = other.is_count_serial;
		count_write = other.count_write;
		count_write_index = other.count_write_index;
		count_read_index = other.count_read_index;
		is_buffer_serial = other.is_buffer_serial;
		buffer_read_index = other.buffer_read_index;
		buffer_write_index = other.buffer_write_index;
		buffer_size = other.buffer_size;
		buffer_pointer = other.buffer_pointer;
		return *this;
	}
	bool canRead()
	{
		return (readStreamPointer && readStreamPointer->tellg() >= 0);
	}
	bool canWrite()
	{
		return (writeStreamPointer && writeStreamPointer->tellp() >= 0);
	}
	bool readBit()
	{
		if (!readStreamPointer)
			return false;
		if (bitsReadReadByte == 0 || bitsReadReadByte == 8)
		{
			if (next_read_offset)
			{
				if (start_read_offset_position + next_read_offset > getReadPosition())
				{
					readStreamPointer->read(&currentReadByte, 1);
					pushReadBytes(&currentReadByte, readStreamPointer->gcount(), 1);
					bitsReadReadByte = 0;
				}
			}
			else
			{
				readStreamPointer->read(&currentReadByte, 1);
				pushReadBytes(&currentReadByte, readStreamPointer->gcount(), 1);
				bitsReadReadByte = 0;
			}
		}
		return currentReadByte & (1 << (bitsReadReadByte++));
	}

	void writeBit(bool bit)
	{
		currentWriteByte |= bit << (bitsWrittenWriteByte++);
		if (bitsWrittenWriteByte == 8)
		{
			writeByte(currentWriteByte);
		}
	}

	template <typename T>
	Serial& operator<<(const T& value)
	{
		if constexpr (std::is_same_v<T, bool>)
		{
			if (bitStream)
			{
				writeBit(value);
			}
			else
			{
				writeByte(value);
			}
			return *this;
		}
		else if constexpr (std::is_trivially_copyable_v<T>)
		{
			return writeBytes((const char*)&value, sizeof(T));
		}
		else if constexpr (requires { serialize(*this, value); })
		{
			return serialize(*this, value);
		}
		throw std::runtime_error("Unable to serialize T");
	}
	template <typename T>
	Serial& operator>>(T& value)
	{
		if constexpr (std::is_same_v<T, bool>)
		{
			if (bitStream)
			{
				value = readBit();
			}
			else
			{
				value = readByte();
			}
		}
		else if constexpr (std::is_trivially_copyable_v<T>)
		{
			if (read_buffer)
				readBytesWithBuffer((char*)&value, sizeof(T), *read_buffer);
			else
				readBytes((char*)&value, sizeof(T));
		}
		else if constexpr (requires { deserialize(*this, value); })
		{
			deserialize(*this, value);
		}
		else
		{
			throw std::runtime_error("Unable to deserialize T");
		}
		return *this;
	}
	/**
	 * @brief Reads a fixed byte size into a destination buffer from the Serial
	 */
	int64_t readBytes(char* dest, size_t size)
	{
		if (is_count_serial)
		{
			count_read_index += size;
			return size;
		}
		if (is_buffer_serial)
		{
			if (buffer_read_index <= buffer_size - (int64_t)size)
			{
				memcpy(dest, buffer_pointer + buffer_read_index, size);
				buffer_read_index += size;
				return size;
			}
			return 0;
		}
		if (!readStreamPointer)
			return 0;
		size_t bytes_read = 0;
		char* dest_ptr = dest;
		if (bitStream)
		{
			for (int i = 0; i < size; i++)
			{
				dest[i] = readByte();
				bytes_read++;
			}
		}
		else
		{
			if (next_read_offset)
			{
				dest_ptr = dest + next_read_offset;
			}
			auto reading_bytes = size - next_read_offset;
			readStreamPointer->read(dest_ptr, reading_bytes);
			bytes_read = readStreamPointer->gcount();
			pushReadBytes(dest_ptr, bytes_read, reading_bytes);
			auto avail = readStreamPointer->rdbuf()->in_avail();
			if (bytes_read != reading_bytes && avail == -1)
			{
				read_eof = true;
			}
			else if ((!bytes_read || bytes_read != reading_bytes) && !avail)
			{
				read_empty = true;
			}
			else
			{
				read_empty = false;
				read_eof = false;
			}
			if (next_read_offset)
			{
				next_read_offset = 0;
				start_read_offset_position = 0;
			}
		}
		// if (bytes_read && m_TicThisValue)
		// {
		// 	auto ptr = (const char*)dest_ptr;
		// 	for (uint32_t i = 0; i < bytes_read; ++i)
		// 	{
		// 		m_SerialValue ^= (uint16_t)ptr[i] << 4;
		// 	}
		// }
		last_bytes_read = bytes_read;
		did_not_read_whole_size();
		return last_bytes_read;
	}
	/**
	 * @brief Reads a fixed byte size into a destination buffer from the Serial, reading from buffer_serial first
	 */
	int64_t readBytesWithBuffer(char* dest, size_t size, Serial& buffer_serial)
	{
		auto buffer_begin_read_pos = buffer_serial.getReadPosition();
		auto buffer_read_ret = buffer_serial.readBytes(dest, size);
		auto buffer_end_read_pos = buffer_begin_read_pos + buffer_read_ret;
		auto buffer_read = (buffer_end_read_pos - buffer_begin_read_pos);
		if (buffer_read)
		{
			if (buffer_read == size)
			{
				_size_mismatch_flag = false;
				return buffer_read;
			}
			next_read_offset = buffer_read;
			start_read_offset_position = getReadPosition();
		}
		auto left_to_read = size - buffer_read;
		int64_t main_read_bytes = 0;
		pushByteReadBuffer();
		main_read_bytes = readBytes(dest, size);
		auto& [popped_bytes_read, popped_bytes_ptr, popped_bytes_index] = peekByteReadBuffer();
		buffer_serial.clearRead();
		if (popped_bytes_read)
		{
			buffer_serial.writeBytes(popped_bytes_ptr, popped_bytes_read);
		}
		buffer_serial.setReadPosition(buffer_begin_read_pos + buffer_read + popped_bytes_read);
		popByteReadBuffer();
		return buffer_read + main_read_bytes;
	}
	/**
	 * @brief Writes a fixed byte size from a src buffer into the Serial
	 */
	Serial& writeBytes(const char* src, size_t size)
	{
		if (is_count_serial)
		{
			count_write += size;
			count_write_index += size;
			return *this;
		}
		if (is_buffer_serial)
		{
			if (buffer_write_index <= buffer_size - (int64_t)size)
			{
				memcpy(buffer_pointer + buffer_write_index, src, size);
				buffer_write_index += size;
			}
			return *this;
		}
		if (!writeStreamPointer)
			return *this;
		if (bitStream)
		{
			for (int i = 0; i < size; i++)
			{
				writeByte(src[i]);
			}
		}
		else
		{
			writeStreamPointer->write(src, size);
		}
		// if (m_TicThisValue)
		// {
		// 	auto ptr = src;
		// 	for (uint32_t i = 0; i < size; ++i)
		// 	{
		// 		m_SerialValue ^= (uint16_t)ptr[i] << 4;
		// 	}
		// }
		return *this;
	}
	/**
	 * @brief Reads bits into bitContainer by accessing at [i] while i < size, expects container to be at least index +
	 * size
	 */
	template <typename BitContainerT>
	Serial& readBits(BitContainerT& bitContainer, size_t index, size_t size)
	{
		if (!bitStream)
		{
			throw std::runtime_error("readBits called and Serial is not a bitStream");
		}
		for (int i = 0; i < size;)
		{
			if (bitsReadReadByte == 8)
			{
				readByte();
			}
			for (size_t k = 0 + bitsReadReadByte; k < 8 && i < size; ++k, ++i, bitsReadReadByte++)
			{
				bitContainer[i + index] = (currentReadByte >> k) & 1;
			}
		}
		return *this;
	}
	/**
	 * @brief Writes bits into the Serial from bitContainer by accessing at [i] while i < size, expects container to be
	 * at least index + size
	 */
	template <typename BitContainerT>
	Serial& writeBits(const BitContainerT& bitContainer, size_t index, size_t size)
	{
		if (!bitStream)
		{
			throw std::runtime_error("writeBits called and Serial is not a bitStream");
		}
		auto bitsToGo = size;
		auto bitsSize = bitContainer.size();
		for (int i = 0; i < size;)
		{
			char bitsThisByte = bitsToGo >= 8 ? 8 : bitsToGo;
			for (size_t k = bitsWrittenWriteByte; k < 8 && i < size; ++k, ++i, ++bitsWrittenWriteByte)
			{
				currentWriteByte |= (bitContainer[i] & 1) << k;
			}
			bitsToGo -= bitsThisByte;
			if (bitsWrittenWriteByte == 8)
			{
				writeByte(currentWriteByte);
			}
		}
		return *this;
	}
	/**
	 * reads a single byte from the Serial
	 */
	char readByte()
	{
		if (bitStream && (bitsReadReadByte > 0 && bitsReadReadByte < 8))
		{
			char byte = 0;
			for (auto e = 0 ; e < 8; e++)
			{
				byte |= (readBit() << e);
			}
			return byte;
		}

		readBytes(&currentReadByte, 1);
		return currentReadByte;
	}
	/**
	 * writes a single byte to the Serial
	 */
	void writeByte(char byte)
	{
		if (bitStream && bitsWrittenWriteByte > 0 && bitsWrittenWriteByte < 8)
		{
			for (char e = 0; e < 8; e++)
			{
				writeBit(byte & (1 << e));
			}
			return;
		}
		writeBytes(&byte, 1);
		currentWriteByte = 0;
		bitsWrittenWriteByte = 0;
	}

	void synchronize()
	{
		if (bitsWrittenWriteByte > 0 && bitsWrittenWriteByte < 8)
		{
			bitsWrittenWriteByte = 8;
			writeByte(currentWriteByte);
		}
		if (writeStreamPointer)
			writeStreamPointer->flush();
		if (readStreamPointer)
			readStreamPointer->sync();
	}
	int64_t getWritePosition()
	{
		if (is_count_serial)
			return count_write_index;
		if (is_buffer_serial)
			return buffer_write_index;
		if (writeStreamPointer)
			return writeStreamPointer->tellp();
		return -1;
	}
	int64_t getReadPosition()
	{
		if (is_count_serial)
			return count_read_index;
		if (is_buffer_serial)
			return buffer_read_index;
		if (readStreamPointer)
			return readStreamPointer->tellg();
		return -1;
	}
	void setWritePosition(size_t index)
	{
		if (is_count_serial)
		{
			count_write_index = index;
			return;
		}
		if (is_buffer_serial)
		{
			buffer_write_index = index;
			return;
		}
		if (writeStreamPointer)
			writeStreamPointer->seekp(index);
	}
	void setReadPosition(size_t index)
	{
		if (is_count_serial)
		{
			count_read_index = index;
			return;
		}
		if (is_buffer_serial)
		{
			buffer_read_index = index;
			return;
		}
		if (readStreamPointer)
			readStreamPointer->seekg(index);
	}
	int64_t getWriteLength() {
		if (is_count_serial)
			return count_write;
		if (is_buffer_serial)
			return buffer_size;
		if (!writeStreamPointer)
			return 0;
		auto orgpos = getWritePosition();
		writeStreamPointer->seekp(0, std::ios::end);
		auto pos = getWritePosition();
		writeStreamPointer->seekp(orgpos, std::ios::beg);
		return pos;
	}
	int64_t getReadLength() {
		if (is_count_serial)
			return count_write;
		if (is_buffer_serial)
			return buffer_size;
		if (!readStreamPointer)
			return 0;
		auto orgpos = getReadPosition();
		readStreamPointer->seekg(0, std::ios::end);
		auto pos = getReadPosition();
		readStreamPointer->seekg(orgpos, std::ios::beg);
		return pos;
	}

	void pushByteReadBuffer()
	{
		currentReadBuffer = {0, nullptr, 0};
		pushedReadBuffer = true;
	}

	void pushReadBytes(const char* data, size_t _size, size_t expected_size)
	{
		if (_size != expected_size)
			_size_mismatch_flag = true;
		if (pushedReadBuffer)
		{
			auto& [size, ptr, index] = currentReadBuffer;
			size += _size;
			if (_size)
			{
				if (ptr)
				{
					ptr = (char*)realloc(ptr, size);
				}
				else
				{
					ptr = (char*)malloc(size);
					memset(ptr, 0, size);
				}
				memcpy(ptr + index, data, _size);
				index += _size;
			}
		}
	}

	void popByteReadBuffer()
	{
		if (!pushedReadBuffer)
		{
			return;
		}
		pushedReadBuffer = false;
		auto& [size, ptr, index] = currentReadBuffer;
		if (ptr)
		{
			free(ptr);
			ptr = nullptr;
		}
		size = 0;
		index = 0;
	}

	std::tuple<size_t, char*, size_t>& peekByteReadBuffer()
	{
		if (!pushedReadBuffer)
		{
			throw std::runtime_error("peeked when empty");
		}
		return currentReadBuffer;
	}

	bool is_read_eof()
	{
		return read_eof;
	}

	bool is_read_empty()
	{
		return read_empty;
	}

	bool did_not_read_whole_size()
	{
		__size_mismatch_flag = _size_mismatch_flag;
		_size_mismatch_flag = false;
		return __size_mismatch_flag;
	}

	bool last_did_not_read_whole_size()
	{
		return __size_mismatch_flag;
	}

	size_t get_last_bytes_read()
	{
		return last_bytes_read;
	}


	template<typename T>
	bool readTypeWithBuffer(T& val, Serial& buffer_serial)
	{
		read_buffer = &buffer_serial;
		(*this) >> val;
		read_buffer = nullptr;
		if (
			(buffer_serial.last_did_not_read_whole_size() || buffer_serial.is_read_empty() || buffer_serial.is_read_eof()) &&
			(last_did_not_read_whole_size() || is_read_empty() || is_read_eof())
		)
		{
			return false;
		}
		return true;
	}

	void clearRead()
	{
		if (readStreamPointer)
		{
			readStreamPointer->clear();
		}
		if (__size_mismatch_flag)
			__size_mismatch_flag = false;
	}
};

#define SERIALIZE_VERSION_FUNC_DECLARE(NAME) auto SERIALIZE_VERSION_##NAME = [&](uint64_t SERIALIZE_VERSION_INT) -> bool {\
	serial << SERIALIZE_VERSION_INT;\
	switch (SERIALIZE_VERSION_INT) {
#define SERIALIZE_VERSION_CASE(VERSION, STATEMENT) case VERSION: STATEMENT
#define SERIALIZE_VERSION_FUNC_END } return true; }

#define DESERIALIZE_VERSION_FUNC_DECLARE(NAME) auto DESERIALIZE_VERSION_##NAME = [&]() -> bool {\
	uint64_t DESERIALIZE_VERSION_INT = 0;\
	serial >> DESERIALIZE_VERSION_INT;\
	switch (DESERIALIZE_VERSION_INT) {
#define DESERIALIZE_VERSION_CASE(VERSION, STATEMENT) case VERSION: STATEMENT
#define DESERIALIZE_VERSION_FUNC_END } return true; }
