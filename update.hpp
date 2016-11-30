#pragma once
#include <net/tcp/connection.hpp>
#include <delegate>
#include <map>

struct buffer_len {
  const char* buffer;
  int length;
};

struct storage_entry;
struct storage_header;

struct Storage
{
  typedef net::tcp::Connection_ptr Connection;
  typedef uint16_t uid;
  
  void add_string(uid, const std::string&);
  void add_buffer(uid, buffer_len);
  void add_buffer(uid, void*, size_t);
  void add_connection(uid, Connection);
  
  
  Storage(storage_header& sh) : hdr(sh) {}
  
private:
  storage_header& hdr;
};

struct Restore
{
  typedef net::tcp::Connection_ptr Connection;
  
  std::string  as_string() const;
  buffer_len   as_buffer() const;
  Connection   as_tcp_connection(net::TCP&) const;
  
  template <typename S>
  inline S as_type() const;
  
  int16_t  get_type() const noexcept;
  uint16_t get_id() const noexcept;
  int      length() const noexcept;
  void*    data() const noexcept;
  
  Restore(storage_entry& e) : ent(e) {}
private:
  storage_entry& ent;
};

template <typename S>
inline S Restore::as_type() const {
  return *(S*) data();
}

struct LiveUpdate
{
  typedef delegate<void(Restore)> resume_func;
  typedef delegate<void(Storage)> storage_func;

  // start a live update process
  static void begin(buffer_len blob, storage_func);
  
  // returns true if there is stored data from before
  static bool is_resumable();
  
  // register handler for a specific id
  static void on_resume(uint16_t id, resume_func);
  
  // attempt to restore existing payloads
  // returns false if there was nothing there
  static bool resume(resume_func);
};
