#ifndef __svstream_H__
#define __svstream_H__

#include <streambuf>
#include <istream>
#include <string_view>

namespace util {
	/** 
	 * string_view-compatible buffer for reading read-only characters from memory.
	 * Implements a buffer based on stackoverflow discussions. Written to be used
	 * via isviewstream.
	 * 
	 * @warning sviewbuf does not own memory. If the lifetime of the string ends
	 * before this buffer, reading from it invokes undefined behavior.
	 * 
	 * @see https://stackoverflow.com/a/13059195
	 * @see https://stackoverflow.com/a/46069245
	 */ 
	class sviewbuf : public std::streambuf {
	private:
		using ios_base = std::ios_base;

	protected:
		pos_type seekoff(off_type off, ios_base::seekdir dir,
						ios_base::openmode which = ios_base::in |
													ios_base::out) override {
			if (dir == ios_base::cur)
				gbump(off);
			else if (dir == ios_base::end)
				setg(eback(), egptr() + off, egptr());
			else if (dir == ios_base::beg)
				setg(eback(), eback() + off, egptr());
			return gptr() - eback();
		}

		pos_type seekpos(pos_type sp, ios_base::openmode which) override {
			return seekoff(sp - pos_type(off_type(0)), ios_base::beg, which);
		}

	public:
		sviewbuf(const char* s, std::size_t count) {
			auto p = const_cast<char*>(s);
			this->setg(p, p, p + count);
		}

		sviewbuf(std::string_view str) :
			sviewbuf(str.data(), str.size()) {
		}
	};


	/** 
	 * A std::stringstream analog for string_view.
	 * Implements a string_view-based input stream based on stackoverflow
	 * discussions.
	 * 
	 * @warning isviewstream does not own memory. If the lifetime of the string
	 * ends before this stream, reading from it invokes undefined behavior.
	 * 
	 * @see https://stackoverflow.com/a/13059195
	 * @see https://stackoverflow.com/a/46069245
	 */ 
	class isviewstream : public std::istream {
	private:
		sviewbuf m_buf;

	public:
		isviewstream(std::string_view str)
			: m_buf(str)
			, std::istream(&m_buf) {}
	};

}

#endif