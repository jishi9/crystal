#ifndef UTILS_H
#define UTILS_H

#include <chrono>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <string>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/gzip_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include "zip/izipfile.h"
#include "zip/ozipfile.h"
#include "zip/zipstream.h"

#include "protogen/mesh.pb.h"
#include "mesh.h"


typedef std::chrono::high_resolution_clock hrclock;

using google::protobuf::io::CodedInputStream;
using google::protobuf::io::CodedOutputStream;
using google::protobuf::io::IstreamInputStream;
using google::protobuf::io::OstreamOutputStream;

using zip::izipstream;
using zip::ozipstream;
using zip::izipfile;
using zip::ozipfile;

using std::cerr;
using std::chrono::duration;
using std::chrono::duration_cast;
using std::chrono::microseconds;
using std::chrono::nanoseconds;
using std::endl;
using std::endl;
using std::locale;
using std::string;
using std::unique_ptr;
using std::to_string;


// Assertion checker
inline static const char* convert(const string& str) {
  return str.c_str();
}

inline static const char* convert(const char* str) {
  return str;
}

template <typename T>
inline static const char* convert(const T n) {
  // Memory leak - but this is used only when an assertion fails, so..
  string* str = new string(to_string(n));
  return convert(*str);
}

template <typename... Arguments>
inline static void check(const bool success, const Arguments&... args) {
#ifdef IGNORE_CHECK
#else
  if (!success) {
    for (const char* message : {convert(args)...}) {
      cerr << message;
    }
    cerr << endl;
    exit(1);
  }
#endif
}


// Iterator that runs in the range ['from', 'to')
class IterAtoB {
  public:
    // Iterator class
    class iterator {
    public:
      iterator(int current) : current(current) {}
      iterator operator+(const int steps) const {
        iterator result(*this);
        result.current += steps;
        return result;
      }
      int operator*() const { return current; }
      int operator++() { return ++current; }
      int operator++(int) { return current++; }
      int operator--() { return --current; }
      int operator--(int) { return current--; }
      bool operator==(const iterator& rhs) const {
        return current == rhs.current;
      }
      bool operator!=(const iterator& rhs) const {
        return current != rhs.current;
      }
    private:
      int current;
    };

    IterAtoB(int from, int just_before) : from(from), just_before(just_before) {}

    iterator begin() const { return iterator(from); }
    iterator end() const { return iterator(just_before); }


  private:
    const int from;
    const int just_before;
};



// Usage:
//    ResourceManager<ClassName> r(arg1, arg2, ...)
//    r.replace(arg1, arg2);
//
template<typename Resource>
class ResourceManager {
  public:
    template<typename... Arguments>
    ResourceManager(Arguments&&... args) {
      replace<Arguments...>(std::forward<Arguments>(args)...);
    }

    ResourceManager() {}

    template<typename... Arguments>
    void replace(Arguments&&... args) {
      resource.reset();
      resource.reset(new Resource(std::forward<Arguments>(args)...));
    }

    void discard() {
      resource.reset();
    }

    operator bool() const {
      return !!resource;
    }

    const Resource* operator->() const {
      return resource.get();
    }

    Resource* operator->() {
      return resource.get();
    }

    const Resource& operator* () const {
      return *resource;
    }

    Resource& operator* () {
      return *resource;
    }

  private:
    unique_ptr<Resource> resource;
};

template<typename Resource, typename... Arguments>
ResourceManager<Resource> make_resource_manager(Arguments... args) {
  return ResourceManager<Resource>(args...);
}


// Output stream used to serialize data
class MeshOutputStream {

  struct OStreamChain {
    OStreamChain(ozipfile& zip_file, const char* filename) :
      ozs(zip_file, filename),
      oos(&ozs),
      cos(&oos) {}
    ozipstream ozs;
    OstreamOutputStream oos;
    CodedOutputStream cos;
  };

  public:
    MeshOutputStream(const char* filename) : zip_file(filename) {
      check(zip_file.isOk(), "Failed to open zipfile ", filename);
    }

  void writeString(const string& str) {
    check(!!zip_entry, "No section opened when writing ", str);
    writeNonNegInt(str.size());
    zip_entry->cos.WriteString(str);
    check(!zip_entry->cos.HadError(), "Write failure - string");
  }


    void writeSection(const string& section_name) {
      zip_entry.replace(zip_file, section_name.c_str());
      check(!!zip_entry, "Failed to open zipentry");
    }

    void writeNonNegInt(const int num) {
      check(num >= 0, "Writing negative integer");

      check(!!zip_entry, "No section opened when writing integer");
      zip_entry->cos.WriteVarint32SignExtended(num);
      check(!zip_entry->cos.HadError(), "Write failure - int");
    }

    template <typename ProtoBuf>
    void writeProtoBuf(const ProtoBuf& pb) {
      // Write message size
      const int message_size = pb.ByteSize();
      writeNonNegInt(message_size);

      // Write message
      check(!!zip_entry, "No section opened when writing proto ", pb.GetTypeName());
      check(pb.SerializeToCodedStream(&zip_entry->cos), "Failed to write message");
      check(!zip_entry->cos.HadError(), "Write failure - protobuf");
    }

  private:
    static ozipfile openOZipFile(const char* filename) {
      ozipfile zip_file(filename);
      check(zip_file.isOk(), "Failed to open zipfile ", filename);
      return zip_file;
    }

    ozipfile zip_file;
    ResourceManager<OStreamChain> zip_entry;
};


// Input stream used to read serialized data
class MeshInputStream {
  struct IStreamChain {
    IStreamChain(izipfile& zip_file, const char* filename) :
      izs(zip_file, filename),
      iis(getIzs(filename)),
      cis(&iis) {
        check(zip_file.isOk(), "Zipfile aint ok..");
      }
    izipstream izs;
    IstreamInputStream iis;
    CodedInputStream cis;

    izipstream* getIzs(const char* filename) {
      check(!izs.bad(), "Failed to open section ", filename);
      return &izs;
    }
  };

  public:
    MeshInputStream(const char* filename) : zip_file(filename) {
      check(zip_file.isOk(), "Failed to open zipfile");
    }

    void readString(const string& expected_name) {
      const int str_len = readNonNegInt();
      check(str_len == expected_name.size(), "Mismatched section size");

      string actual_name;
      check(zip_entry->cis.ReadString(&actual_name, str_len), "Failed to read string");
      check(expected_name.compare(actual_name) == 0, "Mismatched string");
    }

    void readSection(const string& section_name) {
      check(zip_file.isOk(), "zip file not ok just before reading section");
      zip_entry.replace(zip_file, section_name.c_str());
      check(!zip_entry->izs.bad(), "zip entry is bad");
    }

    int readNonNegInt() {
      unsigned int num;
      check(!!zip_entry, "No section opened!");
      check(zip_entry->cis.ReadVarint32(&num), "Failed to read integer");
      int signed_num = static_cast<int>(num);
      check(signed_num >= 0, "Read integer overflows");
      return signed_num;
    }

    template <typename ProtoBufPointer>
    void readProtoBuf(ProtoBufPointer pb) {
      // Read message size
      const int message_size = readNonNegInt();

      // Read message
      check(!!zip_entry, "No section opened!");
      const auto limit = zip_entry->cis.PushLimit(message_size);
      check(pb->MergeFromCodedStream(&zip_entry->cis), "Failed to read protocol buffer");
      check(zip_entry->cis.ConsumedEntireMessage(), "Failed to read protocol buffer (2)");
      zip_entry->cis.PopLimit(limit);
    }

    void skipProtoBuf() {
      // Read message size
      const int message_size = readNonNegInt();

      // Skip message
      check(zip_entry->cis.Skip(message_size), "Failed to skip");
    }

  private:
    static izipfile openIZipFile(const char* filename) {
      izipfile zip_file(filename);
      check(zip_file.isOk(), "Failed to open zipfile");
      return zip_file;
    }

    izipfile zip_file;
    ResourceManager<IStreamChain> zip_entry;
};


// Used to group thousands
struct thousands_grouping : std::numpunct<char> {
  std::string do_grouping() const {return "\03";}
};


// High precision timer
class Timer {
  public:
    Timer() :
      last_clock(hrclock::now()),
      grouping_locale(cerr.getloc(), new thousands_grouping()) {}

    nanoseconds lap() {
      const hrclock::time_point old_time = last_clock;
      last_clock = hrclock::now();
      return duration_cast<nanoseconds>(last_clock - old_time);
    }

    void lapAndPrint(const char* text) {
      const nanoseconds delta = lap();

      const auto old_locale = cerr.imbue(grouping_locale);
      cerr << text << ": " << delta.count() << " ns" << endl;
      cerr.imbue(old_locale);
    }

    hrclock::time_point last_clock;

  private:
    const locale grouping_locale;
};

// Match strings
inline bool match(const char* lhs, const char* rhs) {
  return strcmp(lhs, rhs) == 0;
}

#endif
