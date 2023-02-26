#include <grpcpp/grpcpp.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/completion_queue.h>
#include <grpcpp/impl/codegen/status_code_enum.h>
#include <string>
#include <fstream>
#include "string.h"
#include "afsgrpc.grpc.pb.h"
#include <fcntl.h>

#include "client.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc::ClientReader;
using grpc::ClientWriter;

using afsgrpc::FileReply;
using afsgrpc::FileRequest;
using afsgrpc::AttributeReply;
using afsgrpc::MakeDirRequest;
using afsgrpc::CreateReply;
using afsgrpc::UploadRequest;
using afsgrpc::ReadDirReply;
using afsgrpc::FileService;
using afsgrpc::UnlinkRequest;
using afsgrpc::UnlinkReply;

using namespace std;

const string BASE_DIR = "/users/askagarw/ClientCache/";

class FileServiceClient {
 public:

  void create_stub(std::shared_ptr<Channel> channel) {
    stub_ = FileService::NewStub(channel);
  }

  // Assembles client payload, sends it to the server, and returns its response
  int downloadFile(std::string file_name) {
    // Data to be sent to server
    FileRequest request;
    ClientContext context;
    request.set_file_path(file_name);

    std::unique_ptr<ClientReader<FileReply>> reader(stub_->DownloadFile(&context, request));
    FileReply response;

    string filePathToWrite = BASE_DIR + file_name;
    std::ofstream local_file(filePathToWrite, std::ofstream::binary);

    if (!local_file) {
      std::cerr << "Failed to open local file " << file_name << std::endl;
      return -1;
    }

    while (reader->Read(&response)) {
      local_file.write(response.file_data().data(), response.file_data().size());
    }

    Status status = reader->Finish();
    if (!status.ok()) {
      std::cerr << "Failed to download file: " << status.error_message() << std::endl;
      local_file.close();
      std::remove(filePathToWrite.c_str());  // Remove the incomplete local file
      return -1;
    }

    return 0;

    // // Container for server response
    // FileReply reply;

    // // Context can be used to send meta data to server or modify RPC behaviour
    // ClientContext context;

    // // Actual Remote Procedure Call
    // Status status = stub_->DownloadFile(&context, request, &reply);

    // // Returns results based on RPC status
    // if (status.ok()) {
    //   return reply.file_data();
    // } else {
    //   std::cout << status.error_code() << ": " << status.error_message()
    //             << std::endl;
    //   return "RPC Failed";
    // }
  }

  std::int32_t sendRequestForMkdir(std::string dir_path, int mode) {
    // Data to be sent to server
    MakeDirRequest dirrequest;
    dirrequest.set_dir_path(dir_path);
    dirrequest.set_mode(mode);

    CreateReply dirreply;

    // Context can be used to send meta data to server or modify RPC behaviour
    ClientContext context;

    // Actual Remote Procedure Call
    Status status = stub_->MakeDir(&context, dirrequest, &dirreply);

    // Returns results based on RPC status
    if (status.ok()) {
      return dirreply.err();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return -1;
    }
  }


  std::int32_t sendRequestForUnlink(std::string path, int type) {
    // Data to be sent to server
    UnlinkRequest unlnkReq;
    unlnkReq.set_path(path);
    unlnkReq.set_type(type);

    UnlinkReply unlnkReply;

    // Context can be used to send meta data to server or modify RPC behaviour
    ClientContext context;

    // Actual Remote Procedure Call
    Status status = stub_->Unlink(&context, unlnkReq, &unlnkReply);

    // Returns results based on RPC status
    if (status.ok()) {
      return unlnkReply.err();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return -1;
    }
  }

  int UploadFileToServer(string filePath) {
    string clientFilePath = BASE_DIR + filePath;
    ifstream infile(clientFilePath, std::ifstream::binary);
    if (!infile.is_open()) {
      cerr<< "Couldnt open file";
      return -errno;
    }

    CreateReply reply;
    ClientContext context;
    std::unique_ptr<ClientWriter<UploadRequest>> writer(stub_->UploadFile(&context, &reply));

    const int CHUNK_SIZE = 4096;
    char buffer[CHUNK_SIZE];
    int bytes_read;

    bool happenedOnce = false;
    while ((bytes_read = infile.read(buffer, CHUNK_SIZE).gcount()) > 0) {
      happenedOnce = true;
      UploadRequest chunk;
      chunk.set_file_data(buffer, bytes_read);
      chunk.set_file_path(filePath);
      writer->Write(chunk);
    }

    if (!happenedOnce) {
      UploadRequest chunk;
      cerr<< "Setting file path to :" << filePath<<endl;
      chunk.set_file_path(filePath);
      writer->Write(chunk);
    }

    writer->WritesDone();
    Status status = writer->Finish();
    return status.ok();

    // UploadRequest request;
    // request.set_file_data(content);
    // request.set_file_path(filePath);
    // CreateReply reply;
    // ClientContext context;
    // Status status = stub_->UploadFile(&context, request, &reply);
    // if (status.ok()) {
    //   return reply.err();
    // } else {
    //   std::cout << status.error_code() << ": " << status.error_message()
    //             << std::endl;
    //   return -1;
    // }
  }

  int GetAttributeFromServer(char filePath[], struct stat *stbuf) {
     string filePathTemp(filePath);
    FileRequest request;
    request.set_file_path(filePathTemp);
    ClientContext context;
    AttributeReply reply;
    Status status = stub_->GetAttribute(&context, request, &reply);
    if (status.ok()) {
      stbuf->st_dev = reply.dev();
      stbuf->st_ino = reply.ino();
      stbuf->st_mode = reply.mode();
      stbuf->st_nlink = reply.nlink();
      stbuf->st_uid = reply.uid();
      stbuf->st_gid = reply.gid();
      stbuf->st_rdev = reply.rdev();
      stbuf->st_size = reply.size();
      stbuf->st_atime = reply.atime();
      stbuf->st_mtime = reply.mtime();
      stbuf->st_ctime = reply.ctime();
      stbuf->st_blksize = reply.blksize();
      stbuf->st_blocks = reply.blocks();

      return reply.res();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return -1;
    }
  }

  int readDirectoryFromServer(string filePath, char* dNames[], struct stat *dEntries, int *size) {
    FileRequest request;
    ReadDirReply reply;
    ClientContext context;

    request.set_file_path(filePath);
    Status status = stub_->ReadDir(&context, request, &reply);
    if (status.ok()) {
      if (reply.err() < 0)
        return reply.err();
      *size = reply.inodenumber().size();
      for (int i = 0; i < reply.inodenumber().size(); i++) {
        memset(dEntries + i, 0, sizeof(dEntries[i]));
        dEntries[i].st_ino = reply.inodenumber().Get(i);
	      dEntries[i].st_mode = reply.type().Get(i) << 12;
	      const char* name = reply.name().Get(i).c_str();
        dNames[i] = (char*)malloc(strlen(name)+1);
        strcpy(dNames[i], name);
      }

      return 0;
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return -1;
    }
  }

 private:
  std::unique_ptr<FileService::Stub> stub_;
};

extern "C" int downloadFileFromServer(char filePath[]) {
    // RPC is created and response is stored
    string filePathTemp(filePath);
    return client.downloadFile(filePathTemp);
}

extern "C" int getAttributeFromServer(char filePath[], struct stat *stbuf) {
    return client.GetAttributeFromServer(filePath, stbuf);
}

extern "C" int makeDir(char path[], int mode) {
    std::int32_t response;

    string dirPathTemp(path); // conversion to string
    response = client.sendRequestForMkdir(dirPathTemp, mode);

    return response;
}


extern "C" int unlnk(char path[], int type) {
    std::int32_t response;

    string newPath(path); // conversion to string
    response = client.sendRequestForUnlink(newPath, type);

    return response;
}

extern "C" int readDir(char path[], char* dNames[], struct stat *dEntries, int *size) {
    string dirPathTemp(path); // conversion to string
    return client.readDirectoryFromServer(dirPathTemp, dNames, dEntries, size);
}

extern "C" int uploadFileToServer(char filePath[]) {
  string path(filePath);
  return client.UploadFileToServer(path);

  // string clientFilePath = BASE_DIR + path;
  // ifstream file(clientFilePath);

  // // Check if the file was successfully opened
  // if (!file.is_open()) {
  //   cout << "Error opening file" << endl;
  //   return -errno;
  // }

  // // Read the entire file into a string
  // string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());

  // // Close the file
  // file.close();
  // return client.UploadFileToServer(path, content);
}

extern "C" void initClient() {

    std::string target_address("10.10.1.5:50051");
    std::shared_ptr<Channel> channel = grpc::CreateChannel(target_address,
                          // Indicate when channel is not authenticated
                          grpc::InsecureChannelCredentials());
    
    client.create_stub(channel);
}

FileServiceClient client;
