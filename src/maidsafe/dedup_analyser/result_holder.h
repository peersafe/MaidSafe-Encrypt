/* Copyright (c) 2010 maidsafe.net limited
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
    * Neither the name of the maidsafe.net limited nor the names of its
    contributors may be used to endorse or promote products derived from this
    software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef SRC_RESULT_HOLDER_H_
#define SRC_RESULT_HOLDER_H_

#include <boost/cstdint.hpp>
#include <boost/signals2.hpp>
#include "filesystem_analyser.h"

namespace bs2 = boost::signals2;

namespace maidsafe {

class FilesystemAnalyser;

class ResultHolder {
 public:
  ResultHolder() : on_file_processed_connection_(), on_failure_connection_() {}
  virtual ~ResultHolder() { DisconnectFromFilesystemAnalyser(); }
  void ConnectToFilesystemAnalyser(FilesystemAnalyser *analyser);
  void DisconnectFromFilesystemAnalyser();
  virtual boost::uintmax_t UniqueFileCount() = 0;
  virtual boost::uintmax_t DuplicateFileCount() = 0;
  virtual boost::uintmax_t TotalUniqueSize() = 0;
  virtual boost::uintmax_t TotalDuplicateSize() = 0;
  virtual boost::uintmax_t ErrorsCount() = 0;
 protected:
  virtual void HandleFileProcessed(FileInfo file_info) = 0;
  virtual void HandleFailure(std::string error_message) = 0;
  bs2::connection on_file_processed_connection_, on_failure_connection_;
};

}  // namespace maidsafe

#endif  // SRC_RESULT_HOLDER_H_
