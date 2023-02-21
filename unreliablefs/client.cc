#include <grpcpp/grpcpp.h>
#include <string>
#include <fstream>
#include "afsgrpc.grpc.pb.h"
#include <fcntl.h>

#include "client.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using afsgrpc::FileReply;
using afsgrpc::FileRequest;
using afsgrpc::AttributeReply;
using afsgrpc::MakeDirRequest;
using afsgrpc::CreateReply;
using afsgrpc::UploadRequest;
using afsgrpc::FileService;

using namespace std;

const string BASE_DIR = "/users/askagarw/ClientCache/";

class FileServiceClient {
 public:

  void create_stub(std::shared_ptr<Channel> channel) {
    stub_ = FileService::NewStub(channel);
  }

  // Assembles client payload, sends it to the server, and returns its response
  std::string sendRequest(std::string file_name) {
    // Data to be sent to server
    FileRequest request;
    request.set_file_path(file_name);

    // Container for server response
    FileReply reply;

    // Context can be used to send meta data to server or modify RPC behaviour
    ClientContext context;

    // Actual Remote Procedure Call
    Status status = stub_->DownloadFile(&context, request, &reply);

    // Returns results based on RPC status
    if (status.ok()) {
      return reply.file_data();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC Failed";
    }
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

  int UploadFileToServer(string filePath, string content) {
    UploadRequest request;
    request.set_file_data(content);
    request.set_file_path(filePath);
    CreateReply reply;
    ClientContext context;
    Status status = stub_->UploadFile(&context, request, &reply);
    if (status.ok()) {
      return reply.err();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return -1;
    }
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

 private:
  std::unique_ptr<FileService::Stub> stub_;
};

extern "C" int downloadFileFromServer(char filePath[]) {
    std::string response;

    // RPC is created and response is stored
    string filePathTemp(filePath);
    response = client.sendRequest(filePathTemp);

    // Write it to filepath
    string filePathToWrite = BASE_DIR + filePathTemp;
    ofstream file(filePathToWrite);
    if (!file.is_open()) {
      cout << "Error opening file" << endl;
      return -errno;
    }

    // Write the string to the file
    file << response;
    file.close();

    return 0;
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

extern "C" int readDir(char path[]) {
    std::int32_t response = 0;

    string dirPathTemp(path); // conversion to string
    //response = client.sendRequestForMkdir(dirPathTemp);

    return response;
}

extern "C" int uploadFileToServer(char filePath[]) {
  string path(filePath);

  string clientFilePath = BASE_DIR + path;
  ifstream file(clientFilePath);

  // Check if the file was successfully opened
  if (!file.is_open()) {
    cout << "Error opening file" << endl;
    return -errno;
  }

  // Read the entire file into a string
  string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());

  // Close the file
  file.close();
  return client.UploadFileToServer(path, content);
}

extern "C" void initClient() {

    std::string target_address("0.0.0.0:50051");
    std::shared_ptr<Channel> channel = grpc::CreateChannel(target_address,
                          // Indicate when channel is not authenticated
                          grpc::InsecureChannelCredentials());
    
    client.create_stub(channel);
}

FileServiceClient client;