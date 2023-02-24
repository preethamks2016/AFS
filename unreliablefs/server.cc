#include <grpcpp/grpcpp.h>
#include <string>
#include <fstream>
#include <sys/stat.h>
#include <iostream>
#include <sys/param.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include "afsgrpc.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using afsgrpc::FileReply;
using afsgrpc::FileRequest;
using afsgrpc::AttributeReply;
using afsgrpc::UploadRequest;
using afsgrpc::CreateReply;
using afsgrpc::MakeDirRequest;
using afsgrpc::ReadDirReply;
using afsgrpc::FileService;
using afsgrpc::UnlinkRequest;
using afsgrpc::UnlinkReply;

using namespace std;

const string BASE_DIR = "/users/askagarw/filestore/";

// Server Implementation
class FileServiceImplementation final : public FileService::Service {
  Status DownloadFile(ServerContext* context, const FileRequest* request,
                     FileReply* reply) override {
    // Obtains the original string from the request
    std::string filePath = request->file_path();
    string serverFilePath = BASE_DIR  + filePath; 
    cout<<"File Request Received for: " + serverFilePath<<std::endl;

    // TODO : Read file from local filesystem
    ifstream file(serverFilePath);

    // Check if the file was successfully opened
    if (!file.is_open()) {
      cout << "Error opening file" << endl;
      return Status::OK;
    }

    // Read the entire file into a string
    string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());

    // Close the file
    file.close();

    // Print the contents of the file
    cout << content << endl;


    reply->set_file_data(content);
    return Status::OK;
  }

  Status GetAttribute(ServerContext* context, const FileRequest* request,
                     AttributeReply* reply) override {
    string filePath = request->file_path();
    string serverFilePath = BASE_DIR  + filePath;
    cout<<"GetAttribute Received for: " + serverFilePath<<std::endl;
    struct stat stbuf;
    int res = lstat(serverFilePath.c_str(), &stbuf);
    

    reply->set_dev(stbuf.st_dev);
    reply->set_ino(stbuf.st_ino);
    reply->set_mode(stbuf.st_mode);
    reply->set_nlink(stbuf.st_nlink);
    reply->set_uid(stbuf.st_uid);
    reply->set_gid(stbuf.st_gid);
    reply->set_rdev(stbuf.st_rdev);
    reply->set_size(stbuf.st_size);
    reply->set_atime(stbuf.st_atime);
    reply->set_mtime(stbuf.st_mtime);
    reply->set_ctime(stbuf.st_ctime);
    reply->set_blksize(stbuf.st_blksize);
    reply->set_blocks(stbuf.st_blocks);
    if (res < 0)
      res = -errno;
    else res = 0;
    reply->set_res(res);

    return Status::OK;
  }

  Status MakeDir(ServerContext* context, const MakeDirRequest* request,
                     CreateReply* reply) override {
    // Obtains the original string from the request
    std::string dirPath = request->dir_path();
    int mode = request->mode();
    string serverDirPath = BASE_DIR  + dirPath;
    cout<<"File Request Received for: " + serverDirPath<<std::endl;

    // Creating a directory
    if (mkdir(serverDirPath.c_str(), mode) == -1) {
        cerr << "Error : " << strerror(errno) << endl;
        reply->set_err(-errno);
    }
    else {
      cout << "Directory created\n";
      reply->set_err(0);
    }

    return Status::OK;
  }


  Status Unlink(ServerContext* context, const UnlinkRequest* request,
                     UnlinkReply* reply) override {
    // Obtains the original string from the request
    std::string path = request->path();
    int type = request->type();
    string serverPath = BASE_DIR  + path;
    cout<<"Delete Request Received for: " + serverPath<<std::endl;

    // Creating a directory
    if (unlink(serverPath.c_str()) == -1) {
        cerr << "Error : " << strerror(errno) << endl;
        reply->set_err(-errno);
    }
    else {
      cout << "File/Directory removed\n";
      reply->set_err(0);
    }

    return Status::OK;
  }

  Status UploadFile(ServerContext* context, const UploadRequest* request,
                     CreateReply* reply) override {
    // Obtains the original string from the request
    std::string filePath = request->file_path();
    string fileData = request->file_data();

    string serverFilePath = BASE_DIR  + filePath;
    cout<<"Upload File Request Received for: " + serverFilePath<<std::endl;

    ofstream file(serverFilePath);
    if (!file.is_open()) {
      cout << "Error opening file" << endl;
      reply->set_err(-errno);
      return Status::OK;
    }

    // Write the string to the file
    file << fileData;
    file.close();
    reply->set_err(0);
    return Status::OK;
  }

  Status ReadDir(ServerContext* context, const FileRequest* request,
                  ReadDirReply* reply) override {

    std::string dirPath = request->file_path();
    string serverDirPath = BASE_DIR + dirPath;
    cout<<"Read Directory received for: " + serverDirPath<<endl;
    DIR *dp;
    struct dirent *de;
    
    dp = opendir(serverDirPath.c_str());
    if (dp == NULL) {
      reply->set_err(-errno);
      return Status::OK;
    }

    while ((de = readdir(dp)) != NULL) {
      reply->add_inodenumber(de->d_ino);  
      reply->add_type(de->d_type);
      reply->add_name(de->d_name);
    }

    closedir(dp);
    reply->set_err(0);
    return Status::OK;              
  }

};

void RunServer() {
  std::string server_address("0.0.0.0:50051");
  FileServiceImplementation service;

  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which
  // communication with client takes place
  builder.RegisterService(&service);

  // Assembling the server
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on port: " << server_address << std::endl;

  server->Wait();
}

int main(int argc, char** argv) {
  RunServer();
  return 0;
}
