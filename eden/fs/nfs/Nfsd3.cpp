/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This software may be used and distributed according to the terms of the
 * GNU General Public License version 2.
 */

#ifndef _WIN32

#include "eden/fs/nfs/Nfsd3.h"

#include <folly/Utility.h>
#include <folly/futures/Future.h>
#include "eden/fs/nfs/NfsdRpc.h"
#include "eden/fs/utils/SystemError.h"

namespace facebook::eden {

namespace {
class Nfsd3ServerProcessor final : public RpcServerProcessor {
 public:
  explicit Nfsd3ServerProcessor(
      std::unique_ptr<NfsDispatcher> dispatcher,
      const folly::Logger* straceLogger,
      bool caseSensitive)
      : dispatcher_(std::move(dispatcher)),
        straceLogger_(straceLogger),
        caseSensitive_(caseSensitive) {}

  Nfsd3ServerProcessor(const Nfsd3ServerProcessor&) = delete;
  Nfsd3ServerProcessor(Nfsd3ServerProcessor&&) = delete;
  Nfsd3ServerProcessor& operator=(const Nfsd3ServerProcessor&) = delete;
  Nfsd3ServerProcessor& operator=(Nfsd3ServerProcessor&&) = delete;

  folly::Future<folly::Unit> dispatchRpc(
      folly::io::Cursor deser,
      folly::io::QueueAppender ser,
      uint32_t xid,
      uint32_t progNumber,
      uint32_t progVersion,
      uint32_t procNumber) override;

  folly::Future<folly::Unit>
  null(folly::io::Cursor deser, folly::io::QueueAppender ser, uint32_t xid);
  folly::Future<folly::Unit>
  getattr(folly::io::Cursor deser, folly::io::QueueAppender ser, uint32_t xid);
  folly::Future<folly::Unit>
  setattr(folly::io::Cursor deser, folly::io::QueueAppender ser, uint32_t xid);
  folly::Future<folly::Unit>
  lookup(folly::io::Cursor deser, folly::io::QueueAppender ser, uint32_t xid);
  folly::Future<folly::Unit>
  access(folly::io::Cursor deser, folly::io::QueueAppender ser, uint32_t xid);
  folly::Future<folly::Unit>
  readlink(folly::io::Cursor deser, folly::io::QueueAppender ser, uint32_t xid);
  folly::Future<folly::Unit>
  read(folly::io::Cursor deser, folly::io::QueueAppender ser, uint32_t xid);
  folly::Future<folly::Unit>
  write(folly::io::Cursor deser, folly::io::QueueAppender ser, uint32_t xid);
  folly::Future<folly::Unit>
  create(folly::io::Cursor deser, folly::io::QueueAppender ser, uint32_t xid);
  folly::Future<folly::Unit>
  mkdir(folly::io::Cursor deser, folly::io::QueueAppender ser, uint32_t xid);
  folly::Future<folly::Unit>
  symlink(folly::io::Cursor deser, folly::io::QueueAppender ser, uint32_t xid);
  folly::Future<folly::Unit>
  mknod(folly::io::Cursor deser, folly::io::QueueAppender ser, uint32_t xid);
  folly::Future<folly::Unit>
  remove(folly::io::Cursor deser, folly::io::QueueAppender ser, uint32_t xid);
  folly::Future<folly::Unit>
  rmdir(folly::io::Cursor deser, folly::io::QueueAppender ser, uint32_t xid);
  folly::Future<folly::Unit>
  rename(folly::io::Cursor deser, folly::io::QueueAppender ser, uint32_t xid);
  folly::Future<folly::Unit>
  link(folly::io::Cursor deser, folly::io::QueueAppender ser, uint32_t xid);
  folly::Future<folly::Unit>
  readdir(folly::io::Cursor deser, folly::io::QueueAppender ser, uint32_t xid);
  folly::Future<folly::Unit> readdirplus(
      folly::io::Cursor deser,
      folly::io::QueueAppender ser,
      uint32_t xid);
  folly::Future<folly::Unit>
  fsstat(folly::io::Cursor deser, folly::io::QueueAppender ser, uint32_t xid);
  folly::Future<folly::Unit>
  fsinfo(folly::io::Cursor deser, folly::io::QueueAppender ser, uint32_t xid);
  folly::Future<folly::Unit>
  pathconf(folly::io::Cursor deser, folly::io::QueueAppender ser, uint32_t xid);
  folly::Future<folly::Unit>
  commit(folly::io::Cursor deser, folly::io::QueueAppender ser, uint32_t xid);

 private:
  std::unique_ptr<NfsDispatcher> dispatcher_;
  const folly::Logger* straceLogger_;
  bool caseSensitive_;
};

/**
 * Convert a exception to the appropriate NFS error value.
 */
nfsstat3 exceptionToNfsError(const folly::exception_wrapper& ex) {
  if (auto* err = ex.get_exception<std::system_error>()) {
    if (!isErrnoError(*err)) {
      return nfsstat3::NFS3ERR_SERVERFAULT;
    }

    switch (err->code().value()) {
      case EPERM:
        return nfsstat3::NFS3ERR_PERM;
      case ENOENT:
        return nfsstat3::NFS3ERR_NOENT;
      case EIO:
      case ETXTBSY:
        return nfsstat3::NFS3ERR_IO;
      case ENXIO:
        return nfsstat3::NFS3ERR_NXIO;
      case EACCES:
        return nfsstat3::NFS3ERR_ACCES;
      case EEXIST:
        return nfsstat3::NFS3ERR_EXIST;
      case EXDEV:
        return nfsstat3::NFS3ERR_XDEV;
      case ENODEV:
        return nfsstat3::NFS3ERR_NODEV;
      case ENOTDIR:
        return nfsstat3::NFS3ERR_NOTDIR;
      case EISDIR:
        return nfsstat3::NFS3ERR_ISDIR;
      case EINVAL:
        return nfsstat3::NFS3ERR_INVAL;
      case EFBIG:
        return nfsstat3::NFS3ERR_FBIG;
      case EROFS:
        return nfsstat3::NFS3ERR_ROFS;
      case EMLINK:
        return nfsstat3::NFS3ERR_MLINK;
      case ENAMETOOLONG:
        return nfsstat3::NFS3ERR_NAMETOOLONG;
      case ENOTEMPTY:
        return nfsstat3::NFS3ERR_NOTEMPTY;
      case EDQUOT:
        return nfsstat3::NFS3ERR_DQUOT;
      case ESTALE:
        return nfsstat3::NFS3ERR_STALE;
      case ETIMEDOUT:
      case EAGAIN:
      case ENOMEM:
        return nfsstat3::NFS3ERR_JUKEBOX;
      case ENOTSUP:
        return nfsstat3::NFS3ERR_NOTSUPP;
      case ENFILE:
        return nfsstat3::NFS3ERR_SERVERFAULT;
    }
    return nfsstat3::NFS3ERR_SERVERFAULT;
  } else if (ex.get_exception<folly::FutureTimeout>()) {
    return nfsstat3::NFS3ERR_JUKEBOX;
  } else {
    return nfsstat3::NFS3ERR_SERVERFAULT;
  }
}

folly::Future<folly::Unit> Nfsd3ServerProcessor::null(
    folly::io::Cursor /*deser*/,
    folly::io::QueueAppender ser,
    uint32_t xid) {
  serializeReply(ser, accept_stat::SUCCESS, xid);
  return folly::unit;
}

/**
 * Convert the POSIX mode to NFS file type.
 */
ftype3 modeToFtype3(mode_t mode) {
  if (S_ISREG(mode)) {
    return ftype3::NF3REG;
  } else if (S_ISDIR(mode)) {
    return ftype3::NF3DIR;
  } else if (S_ISBLK(mode)) {
    return ftype3::NF3BLK;
  } else if (S_ISCHR(mode)) {
    return ftype3::NF3CHR;
  } else if (S_ISLNK(mode)) {
    return ftype3::NF3LNK;
  } else if (S_ISSOCK(mode)) {
    return ftype3::NF3SOCK;
  } else {
    XDCHECK(S_ISFIFO(mode));
    return ftype3::NF3FIFO;
  }
}

/**
 * Convert the POSIX mode to NFS mode.
 *
 * TODO(xavierd): For now, the owner always has RW access, the group R access
 * and others no access.
 */
uint32_t modeToNfsMode(mode_t mode) {
  return kReadOwnerBit | kWriteOwnerBit | kReadGroupBit |
      ((mode & S_IXUSR) ? kExecOwnerBit : 0);
}

/**
 * Convert a POSIX timespec to an NFS time.
 */
nfstime3 timespecToNfsTime(const struct timespec& time) {
  return nfstime3{
      folly::to_narrow(folly::to_unsigned(time.tv_sec)),
      folly::to_narrow(folly::to_unsigned(time.tv_nsec))};
}

fattr3 statToFattr3(const struct stat& stat) {
  return fattr3{
      /*type*/ modeToFtype3(stat.st_mode),
      /*mode*/ modeToNfsMode(stat.st_mode),
      /*nlink*/ folly::to_narrow(stat.st_nlink),
      /*uid*/ stat.st_uid,
      /*gid*/ stat.st_gid,
      /*size*/ folly::to_unsigned(stat.st_size),
      /*used*/ folly::to_unsigned(stat.st_blocks) * 512u,
      /*rdev*/ specdata3{0, 0}, // TODO(xavierd)
      /*fsid*/ folly::to_unsigned(stat.st_dev),
      /*fileid*/ stat.st_ino,
#ifdef __linux__
      /*atime*/ timespecToNfsTime(stat.st_atim),
      /*mtime*/ timespecToNfsTime(stat.st_mtim),
      /*ctime*/ timespecToNfsTime(stat.st_ctim),
#else
      /*atime*/ timespecToNfsTime(stat.st_atimespec),
      /*mtime*/ timespecToNfsTime(stat.st_mtimespec),
      /*ctime*/ timespecToNfsTime(stat.st_ctimespec),
#endif
  };
}

post_op_attr statToPostOpAttr(folly::Try<struct stat>&& stat) {
  if (stat.hasException()) {
    return post_op_attr{};
  } else {
    return post_op_attr{statToFattr3(stat.value())};
  }
}

pre_op_attr statToPreOpAttr(struct stat& stat) {
  return pre_op_attr{wcc_attr{
      /*size*/ folly::to_unsigned(stat.st_size),
#ifdef __linux__
      /*mtime*/ timespecToNfsTime(stat.st_mtim),
      /*ctime*/ timespecToNfsTime(stat.st_ctim),
#else
      /*mtime*/ timespecToNfsTime(stat.st_mtimespec),
      /*ctime*/ timespecToNfsTime(stat.st_ctimespec),
#endif
  }};
}

folly::Future<folly::Unit> Nfsd3ServerProcessor::getattr(
    folly::io::Cursor deser,
    folly::io::QueueAppender ser,
    uint32_t xid) {
  serializeReply(ser, accept_stat::SUCCESS, xid);

  auto args = XdrTrait<GETATTR3args>::deserialize(deser);

  // TODO(xavierd): make an NfsRequestContext.
  static auto context =
      ObjectFetchContext::getNullContextWithCauseDetail("getattr");

  return dispatcher_->getattr(args.object.ino, *context)
      .thenTry([ser = std::move(ser)](folly::Try<struct stat>&& try_) mutable {
        if (try_.hasException()) {
          GETATTR3res res{
              {{exceptionToNfsError(try_.exception()), std::monostate{}}}};
          XdrTrait<GETATTR3res>::serialize(ser, res);
        } else {
          auto stat = std::move(try_).value();

          GETATTR3res res{
              {{nfsstat3::NFS3_OK, GETATTR3resok{statToFattr3(stat)}}}};
          XdrTrait<GETATTR3res>::serialize(ser, res);
        }

        return folly::unit;
      });
}

folly::Future<folly::Unit> Nfsd3ServerProcessor::setattr(
    folly::io::Cursor /*deser*/,
    folly::io::QueueAppender ser,
    uint32_t xid) {
  serializeReply(ser, accept_stat::PROC_UNAVAIL, xid);
  return folly::unit;
}

folly::Future<folly::Unit> Nfsd3ServerProcessor::lookup(
    folly::io::Cursor deser,
    folly::io::QueueAppender ser,
    uint32_t xid) {
  serializeReply(ser, accept_stat::SUCCESS, xid);

  auto args = XdrTrait<LOOKUP3args>::deserialize(deser);

  // TODO(xavierd): make an NfsRequestContext.
  static auto context =
      ObjectFetchContext::getNullContextWithCauseDetail("lookup");

  // TODO(xavierd): the lifetime of this future is a bit tricky and it needs to
  // be consumed in this function to avoid use-after-free. This future may also
  // need to be executed after the lookup call to conform to fill the "post-op"
  // attributes
  auto dirAttrFut = dispatcher_->getattr(args.what.dir.ino, *context);

  if (args.what.name.length() > NAME_MAX) {
    // The filename is too long, let's try to get the attributes of the
    // directory and fail.
    return std::move(dirAttrFut)
        .thenTry([ser =
                      std::move(ser)](folly::Try<struct stat>&& try_) mutable {
          if (try_.hasException()) {
            LOOKUP3res res{
                {{nfsstat3::NFS3ERR_NAMETOOLONG,
                  LOOKUP3resfail{post_op_attr{}}}}};
            XdrTrait<LOOKUP3res>::serialize(ser, res);
          } else {
            LOOKUP3res res{
                {{nfsstat3::NFS3ERR_NAMETOOLONG,
                  LOOKUP3resfail{post_op_attr{statToFattr3(try_.value())}}}}};
            XdrTrait<LOOKUP3res>::serialize(ser, res);
          }

          return folly::unit;
        });
  }

  return folly::makeFutureWith([this, args = std::move(args)]() mutable {
           if (args.what.name == ".") {
             return dispatcher_->getattr(args.what.dir.ino, *context)
                 .thenValue(
                     [ino = args.what.dir.ino](struct stat && stat)
                         -> std::tuple<InodeNumber, struct stat> {
                       return {ino, std::move(stat)};
                     });
           } else if (args.what.name == "..") {
             return dispatcher_->getParent(args.what.dir.ino, *context)
                 .thenValue([this](InodeNumber ino) {
                   return dispatcher_->getattr(ino, *context)
                       .thenValue(
                           [ino](struct stat && stat)
                               -> std::tuple<InodeNumber, struct stat> {
                             return {ino, std::move(stat)};
                           });
                 });
           } else {
             return dispatcher_->lookup(
                 args.what.dir.ino, PathComponent(args.what.name), *context);
           }
         })
      .thenTry([ser = std::move(ser), dirAttrFut = std::move(dirAttrFut)](
                   folly::Try<std::tuple<InodeNumber, struct stat>>&&
                       lookupTry) mutable {
        return std::move(dirAttrFut)
            .thenTry([ser = std::move(ser), lookupTry = std::move(lookupTry)](
                         folly::Try<struct stat>&& dirStat) mutable {
              if (lookupTry.hasException()) {
                LOOKUP3res res{
                    {{exceptionToNfsError(lookupTry.exception()),
                      LOOKUP3resfail{statToPostOpAttr(std::move(dirStat))}}}};
                XdrTrait<LOOKUP3res>::serialize(ser, res);
              } else {
                auto& [ino, stat] = lookupTry.value();
                LOOKUP3res res{
                    {{nfsstat3::NFS3_OK,
                      LOOKUP3resok{
                          /*object*/ nfs_fh3{ino},
                          /*obj_attributes*/
                          post_op_attr{statToFattr3(stat)},
                          /*dir_attributes*/
                          statToPostOpAttr(std::move(dirStat)),
                      }}}};
                XdrTrait<LOOKUP3res>::serialize(ser, res);
              }
              return folly::unit;
            });
      });
}

uint32_t getEffectiveAccessRights(
    const struct stat& /*stat*/,
    uint32_t desiredAccess) {
  // TODO(xavierd): we should look at the uid/gid of the user doing the
  // request. This should be part of the RPC credentials.
  return desiredAccess;
}

folly::Future<folly::Unit> Nfsd3ServerProcessor::access(
    folly::io::Cursor deser,
    folly::io::QueueAppender ser,
    uint32_t xid) {
  serializeReply(ser, accept_stat::SUCCESS, xid);

  auto args = XdrTrait<ACCESS3args>::deserialize(deser);

  // TODO(xavierd): make an NfsRequestContext.
  static auto context =
      ObjectFetchContext::getNullContextWithCauseDetail("access");

  return dispatcher_->getattr(args.object.ino, *context)
      .thenTry([ser = std::move(ser), desiredAccess = args.access](
                   folly::Try<struct stat>&& try_) mutable {
        if (try_.hasException()) {
          ACCESS3res res{
              {{exceptionToNfsError(try_.exception()),
                ACCESS3resfail{post_op_attr{}}}}};
          XdrTrait<ACCESS3res>::serialize(ser, res);
        } else {
          auto stat = std::move(try_).value();

          ACCESS3res res{
              {{nfsstat3::NFS3_OK,
                ACCESS3resok{
                    post_op_attr{statToFattr3(stat)},
                    /*access*/ getEffectiveAccessRights(stat, desiredAccess),
                }}}};
          XdrTrait<ACCESS3res>::serialize(ser, res);
        }

        return folly::unit;
      });
}

folly::Future<folly::Unit> Nfsd3ServerProcessor::readlink(
    folly::io::Cursor deser,
    folly::io::QueueAppender ser,
    uint32_t xid) {
  serializeReply(ser, accept_stat::SUCCESS, xid);

  auto args = XdrTrait<READLINK3args>::deserialize(deser);

  static auto context =
      ObjectFetchContext::getNullContextWithCauseDetail("readlink");

  auto getattr = dispatcher_->getattr(args.symlink.ino, *context);
  return dispatcher_->readlink(args.symlink.ino, *context)
      .thenTry([ser = std::move(ser), getattr = std::move(getattr)](
                   folly::Try<std::string> tryReadlink) mutable {
        return std::move(getattr).thenTry(
            [ser = std::move(ser), tryReadlink = std::move(tryReadlink)](
                folly::Try<struct stat> tryAttr) mutable {
              if (tryReadlink.hasException()) {
                READLINK3res res{
                    {{exceptionToNfsError(tryReadlink.exception()),
                      READLINK3resfail{statToPostOpAttr(std::move(tryAttr))}}}};
                XdrTrait<READLINK3res>::serialize(ser, res);
              } else {
                auto link = std::move(tryReadlink).value();

                READLINK3res res{
                    {{nfsstat3::NFS3_OK,
                      READLINK3resok{
                          /*symlink_attributes*/ statToPostOpAttr(
                              std::move(tryAttr)),
                          /*data*/ std::move(link),
                      }}}};
                XdrTrait<READLINK3res>::serialize(ser, res);
              }

              return folly::unit;
            });
      });
}

folly::Future<folly::Unit> Nfsd3ServerProcessor::read(
    folly::io::Cursor /*deser*/,
    folly::io::QueueAppender ser,
    uint32_t xid) {
  serializeReply(ser, accept_stat::PROC_UNAVAIL, xid);
  return folly::unit;
}

/**
 * Generate a unique per-EdenFS instance write cookie.
 *
 * TODO(xavierd): Note that for now this will always be 0 as this is to handle
 * the case where the server restart while the client isn't aware.
 */
writeverf3 makeWriteVerf() {
  return 0;
}

folly::Future<folly::Unit> Nfsd3ServerProcessor::write(
    folly::io::Cursor deser,
    folly::io::QueueAppender ser,
    uint32_t xid) {
  serializeReply(ser, accept_stat::SUCCESS, xid);

  auto args = XdrTrait<WRITE3args>::deserialize(deser);

  static auto context =
      ObjectFetchContext::getNullContextWithCauseDetail("write");

  // I have no idea why NFS sent us data that we shouldn't write to the file,
  // but here it is, let's only take up to count bytes from the data.
  auto queue = folly::IOBufQueue();
  queue.append(std::move(args.data));
  auto data = queue.split(args.count);

  return dispatcher_
      ->write(args.file.ino, std::move(data), args.offset, *context)
      .thenTry([ser = std::move(ser)](
                   folly::Try<NfsDispatcher::WriteRes> writeTry) mutable {
        if (writeTry.hasException()) {
          WRITE3res res{
              {{exceptionToNfsError(writeTry.exception()), WRITE3resfail{}}}};
          XdrTrait<WRITE3res>::serialize(ser, res);
        } else {
          auto writeRes = std::move(writeTry).value();

          // NFS is limited to writing a maximum of 4GB (2^32) of data
          // per write call, so despite write returning a size_t, it
          // should always fit in a uint32_t.
          XDCHECK_LE(
              writeRes.written, size_t{std::numeric_limits<uint32_t>::max()});

          WRITE3res res{
              {{nfsstat3::NFS3_OK,
                WRITE3resok{
                    wcc_data{
                        /*before*/ writeRes.preStat.has_value()
                            ? statToPreOpAttr(writeRes.preStat.value())
                            : pre_op_attr{},
                        /*after*/ writeRes.postStat.has_value()
                            ? post_op_attr{statToFattr3(
                                  writeRes.postStat.value())}
                            : post_op_attr{}},
                    /*count*/ folly::to_narrow(writeRes.written),
                    // TODO(xavierd): the following is a total lie and we
                    // should call inode->fdatasync() in the case where
                    // args.stable is anything other than
                    // stable_how::UNSTABLE. For testing purpose, this is
                    // OK.
                    /*committed*/ stable_how::FILE_SYNC,
                    /*verf*/ makeWriteVerf(),
                }}}};
          XdrTrait<WRITE3res>::serialize(ser, res);
        }

        return folly::unit;
      });
}

/**
 * Test if the exception was raised due to a EEXIST condition.
 */
bool isEexist(const folly::exception_wrapper& ex) {
  if (auto* err = ex.get_exception<std::system_error>()) {
    return isErrnoError(*err) && err->code().value() == EEXIST;
  }
  return false;
}

folly::Future<folly::Unit> Nfsd3ServerProcessor::create(
    folly::io::Cursor deser,
    folly::io::QueueAppender ser,
    uint32_t xid) {
  serializeReply(ser, accept_stat::SUCCESS, xid);

  auto args = XdrTrait<CREATE3args>::deserialize(deser);

  static auto context =
      ObjectFetchContext::getNullContextWithCauseDetail("create");

  if (args.how.tag == createmode3::EXCLUSIVE) {
    // Exclusive file creation is complicated, for now let's not support it.
    CREATE3res res{{{nfsstat3::NFS3ERR_NOTSUPP, CREATE3resfail{wcc_data{}}}}};
    XdrTrait<CREATE3res>::serialize(ser, res);
    return folly::unit;
  }

  auto& attr = std::get<sattr3>(args.how.v);

  // If the mode isn't set, make it writable by the owner, readable by the
  // group and other. This is consistent with creating a file with a default
  // umask of 022.
  auto mode =
      attr.mode.tag ? std::get<uint32_t>(attr.mode.v) : (S_IFREG | 0644);

  return dispatcher_
      ->create(
          args.where.dir.ino, PathComponent{args.where.name}, mode, *context)
      .thenTry([ser = std::move(ser), createmode = args.how.tag](
                   folly::Try<NfsDispatcher::CreateRes> try_) mutable {
        if (try_.hasException()) {
          if (createmode == createmode3::UNCHECKED &&
              isEexist(try_.exception())) {
            XLOG(WARN) << "Unchecked file creation returned EEXIST";
            // A file already exist at that location, since this is an
            // UNCHECKED creation, just pretend the file was created just fine.
            // Since no fields are populated, this forces the client to issue a
            // LOOKUP RPC to gather the InodeNumber and attributes for this
            // file. This is probably fine as creating a file that already
            // exists should be a rare event.
            // TODO(xavierd): We should change the file attributes based on
            // the requested args.how.obj_attributes.
            CREATE3res res{
                {{nfsstat3::NFS3_OK,
                  CREATE3resok{
                      /*obj*/ post_op_fh3{},
                      /*obj_attributes*/ post_op_attr{},
                      wcc_data{
                          /*before*/ pre_op_attr{},
                          /*after*/ post_op_attr{},
                      }}}}};
            XdrTrait<CREATE3res>::serialize(ser, res);
          } else {
            CREATE3res res{
                {{exceptionToNfsError(try_.exception()), CREATE3resfail{}}}};
            XdrTrait<CREATE3res>::serialize(ser, res);
          }
        } else {
          auto createRes = std::move(try_).value();

          CREATE3res res{
              {{nfsstat3::NFS3_OK,
                CREATE3resok{
                    /*obj*/ post_op_fh3{nfs_fh3{createRes.ino}},
                    /*obj_attributes*/
                    post_op_attr{statToFattr3(createRes.stat)},
                    wcc_data{
                        /*before*/ createRes.preDirStat.has_value()
                            ? statToPreOpAttr(createRes.preDirStat.value())
                            : pre_op_attr{},
                        /*after*/ createRes.postDirStat.has_value()
                            ? post_op_attr{statToFattr3(
                                  createRes.postDirStat.value())}
                            : post_op_attr{},
                    }}}}};
          XdrTrait<CREATE3res>::serialize(ser, res);
        }
        return folly::unit;
      });
}

folly::Future<folly::Unit> Nfsd3ServerProcessor::mkdir(
    folly::io::Cursor deser,
    folly::io::QueueAppender ser,
    uint32_t xid) {
  serializeReply(ser, accept_stat::SUCCESS, xid);

  auto args = XdrTrait<MKDIR3args>::deserialize(deser);

  static auto context =
      ObjectFetchContext::getNullContextWithCauseDetail("mkdir");

  // Don't allow creating this directory and its parent.
  if (args.where.name == "." || args.where.name == "..") {
    MKDIR3res res{{{nfsstat3::NFS3ERR_EXIST, MKDIR3resfail{}}}};
    XdrTrait<MKDIR3res>::serialize(ser, res);
    return folly::unit;
  }

  // If the mode isn't set, make it writable by the owner, readable by the
  // group and traversable by other.
  auto mode = args.attributes.mode.tag
      ? std::get<uint32_t>(args.attributes.mode.v)
      : (S_IFDIR | 0751);

  // TODO(xavierd): For now, all the other args.attributes are ignored, is it
  // OK?

  return dispatcher_
      ->mkdir(
          args.where.dir.ino, PathComponent{args.where.name}, mode, *context)
      .thenTry([ser = std::move(ser)](
                   folly::Try<NfsDispatcher::MkdirRes> try_) mutable {
        if (try_.hasException()) {
          MKDIR3res res{
              {{exceptionToNfsError(try_.exception()), MKDIR3resfail{}}}};
          XdrTrait<MKDIR3res>::serialize(ser, res);
        } else {
          auto mkdirRes = std::move(try_).value();

          MKDIR3res res{
              {{nfsstat3::NFS3_OK,
                MKDIR3resok{
                    /*obj*/ post_op_fh3{nfs_fh3{mkdirRes.ino}},
                    /*obj_attributes*/
                    post_op_attr{statToFattr3(mkdirRes.stat)},
                    wcc_data{
                        /*before*/ mkdirRes.preDirStat.has_value()
                            ? statToPreOpAttr(mkdirRes.preDirStat.value())
                            : pre_op_attr{},
                        /*after*/ mkdirRes.postDirStat.has_value()
                            ? post_op_attr{statToFattr3(
                                  mkdirRes.postDirStat.value())}
                            : post_op_attr{},
                    }}}}};
          XdrTrait<MKDIR3res>::serialize(ser, res);
        }
        return folly::unit;
      });
}

folly::Future<folly::Unit> Nfsd3ServerProcessor::symlink(
    folly::io::Cursor /*deser*/,
    folly::io::QueueAppender ser,
    uint32_t xid) {
  serializeReply(ser, accept_stat::PROC_UNAVAIL, xid);
  return folly::unit;
}

folly::Future<folly::Unit> Nfsd3ServerProcessor::mknod(
    folly::io::Cursor /*deser*/,
    folly::io::QueueAppender ser,
    uint32_t xid) {
  serializeReply(ser, accept_stat::PROC_UNAVAIL, xid);
  return folly::unit;
}

folly::Future<folly::Unit> Nfsd3ServerProcessor::remove(
    folly::io::Cursor /*deser*/,
    folly::io::QueueAppender ser,
    uint32_t xid) {
  serializeReply(ser, accept_stat::PROC_UNAVAIL, xid);
  return folly::unit;
}

folly::Future<folly::Unit> Nfsd3ServerProcessor::rmdir(
    folly::io::Cursor /*deser*/,
    folly::io::QueueAppender ser,
    uint32_t xid) {
  serializeReply(ser, accept_stat::PROC_UNAVAIL, xid);
  return folly::unit;
}

folly::Future<folly::Unit> Nfsd3ServerProcessor::rename(
    folly::io::Cursor /*deser*/,
    folly::io::QueueAppender ser,
    uint32_t xid) {
  serializeReply(ser, accept_stat::PROC_UNAVAIL, xid);
  return folly::unit;
}

folly::Future<folly::Unit> Nfsd3ServerProcessor::link(
    folly::io::Cursor deser,
    folly::io::QueueAppender ser,
    uint32_t xid) {
  serializeReply(ser, accept_stat::SUCCESS, xid);

  auto args = XdrTrait<LINK3args>::deserialize(deser);

  static auto context =
      ObjectFetchContext::getNullContextWithCauseDetail("link");

  // EdenFS doesn't support hardlinks, let's just collect the attributes for
  // the file and fail.
  return dispatcher_->getattr(args.file.ino, *context)
      .thenTry([ser = std::move(ser)](folly::Try<struct stat> try_) mutable {
        LINK3res res{
            {{nfsstat3::NFS3ERR_NOTSUPP,
              LINK3resfail{statToPostOpAttr(std::move(try_)), wcc_data{}}}}};
        XdrTrait<LINK3res>::serialize(ser, res);
        return folly::unit;
      });
}

folly::Future<folly::Unit> Nfsd3ServerProcessor::readdir(
    folly::io::Cursor /*deser*/,
    folly::io::QueueAppender ser,
    uint32_t xid) {
  serializeReply(ser, accept_stat::PROC_UNAVAIL, xid);
  return folly::unit;
}

folly::Future<folly::Unit> Nfsd3ServerProcessor::readdirplus(
    folly::io::Cursor /*deser*/,
    folly::io::QueueAppender ser,
    uint32_t xid) {
  serializeReply(ser, accept_stat::PROC_UNAVAIL, xid);
  return folly::unit;
}

folly::Future<folly::Unit> Nfsd3ServerProcessor::fsstat(
    folly::io::Cursor deser,
    folly::io::QueueAppender ser,
    uint32_t xid) {
  serializeReply(ser, accept_stat::SUCCESS, xid);

  auto args = XdrTrait<FSSTAT3args>::deserialize(deser);

  static auto context =
      ObjectFetchContext::getNullContextWithCauseDetail("fsstat");

  return dispatcher_->statfs(args.fsroot.ino, *context)
      .thenTry([this, ser = std::move(ser), ino = args.fsroot.ino](
                   folly::Try<struct statfs> statFsTry) mutable {
        return dispatcher_->getattr(ino, *context)
            .thenTry([ser = std::move(ser), statFsTry = std::move(statFsTry)](
                         folly::Try<struct stat> statTry) mutable {
              if (statFsTry.hasException()) {
                FSSTAT3res res{
                    {{exceptionToNfsError(statFsTry.exception()),
                      FSSTAT3resfail{statToPostOpAttr(std::move(statTry))}}}};
                XdrTrait<FSSTAT3res>::serialize(ser, res);
              } else {
                auto statfs = std::move(statFsTry).value();

                FSSTAT3res res{
                    {{nfsstat3::NFS3_OK,
                      FSSTAT3resok{
                          /*obj_attributes*/ statToPostOpAttr(
                              std::move(statTry)),
                          /*tbytes*/ statfs.f_blocks * statfs.f_bsize,
                          /*fbytes*/ statfs.f_bfree * statfs.f_bsize,
                          /*abytes*/ statfs.f_bavail * statfs.f_bavail,
                          /*tfiles*/ statfs.f_files,
                          /*ffiles*/ statfs.f_ffree,
                          /*afiles*/ statfs.f_ffree,
                          /*invarsec*/ 0,
                      }}}};
                XdrTrait<FSSTAT3res>::serialize(ser, res);
              }

              return folly::unit;
            });
      });
}

folly::Future<folly::Unit> Nfsd3ServerProcessor::fsinfo(
    folly::io::Cursor deser,
    folly::io::QueueAppender ser,
    uint32_t xid) {
  serializeReply(ser, accept_stat::SUCCESS, xid);

  auto args = XdrTrait<FSINFO3args>::deserialize(deser);
  (void)args;

  FSINFO3res res{
      {{nfsstat3::NFS3_OK,
        FSINFO3resok{
            // TODO(xavierd): fill the post_op_attr and check the values chosen
            // randomly below.
            post_op_attr{},
            /*rtmax=*/1024 * 1024,
            /*rtpref=*/1024 * 1024,
            /*rtmult=*/1,
            /*wtmax=*/1024 * 1024,
            /*wtpref=*/1024 * 1024,
            /*wtmult=*/1,
            /*dtpref=*/1024 * 1024,
            /*maxfilesize=*/std::numeric_limits<uint64_t>::max(),
            nfstime3{0, 1},
            /*properties*/ FSF3_SYMLINK | FSF3_HOMOGENEOUS | FSF3_CANSETTIME,
        }}}};

  XdrTrait<FSINFO3res>::serialize(ser, res);

  return folly::unit;
}

folly::Future<folly::Unit> Nfsd3ServerProcessor::pathconf(
    folly::io::Cursor deser,
    folly::io::QueueAppender ser,
    uint32_t xid) {
  serializeReply(ser, accept_stat::SUCCESS, xid);

  auto args = XdrTrait<PATHCONF3args>::deserialize(deser);
  (void)args;

  PATHCONF3res res{
      {{nfsstat3::NFS3_OK,
        PATHCONF3resok{
            // TODO(xavierd): fill up the post_op_attr
            post_op_attr{},
            /*linkmax=*/0,
            /*name_max=*/NAME_MAX,
            /*no_trunc=*/true,
            /*chown_restricted=*/true,
            /*case_insensitive=*/!caseSensitive_,
            /*case_preserving=*/true,
        }}}};

  XdrTrait<PATHCONF3res>::serialize(ser, res);

  return folly::unit;
}

folly::Future<folly::Unit> Nfsd3ServerProcessor::commit(
    folly::io::Cursor /*deser*/,
    folly::io::QueueAppender ser,
    uint32_t xid) {
  serializeReply(ser, accept_stat::PROC_UNAVAIL, xid);
  return folly::unit;
}

using Handler = folly::Future<folly::Unit> (Nfsd3ServerProcessor::*)(
    folly::io::Cursor deser,
    folly::io::QueueAppender ser,
    uint32_t xid);

struct HandlerEntry {
  constexpr HandlerEntry() = default;
  constexpr HandlerEntry(folly::StringPiece n, Handler h)
      : name(n), handler(h) {}

  folly::StringPiece name;
  Handler handler = nullptr;
};

constexpr auto kNfs3dHandlers = [] {
  std::array<HandlerEntry, 22> handlers;
  handlers[folly::to_underlying(nfsv3Procs::null)] = {
      "NULL", &Nfsd3ServerProcessor::null};
  handlers[folly::to_underlying(nfsv3Procs::getattr)] = {
      "GETATTR", &Nfsd3ServerProcessor::getattr};
  handlers[folly::to_underlying(nfsv3Procs::setattr)] = {
      "SETATTR", &Nfsd3ServerProcessor::setattr};
  handlers[folly::to_underlying(nfsv3Procs::lookup)] = {
      "LOOKUP", &Nfsd3ServerProcessor::lookup};
  handlers[folly::to_underlying(nfsv3Procs::access)] = {
      "ACCESS", &Nfsd3ServerProcessor::access};
  handlers[folly::to_underlying(nfsv3Procs::readlink)] = {
      "READLINK", &Nfsd3ServerProcessor::readlink};
  handlers[folly::to_underlying(nfsv3Procs::read)] = {
      "READ", &Nfsd3ServerProcessor::read};
  handlers[folly::to_underlying(nfsv3Procs::write)] = {
      "WRITE", &Nfsd3ServerProcessor::write};
  handlers[folly::to_underlying(nfsv3Procs::create)] = {
      "CREATE", &Nfsd3ServerProcessor::create};
  handlers[folly::to_underlying(nfsv3Procs::mkdir)] = {
      "MKDIR", &Nfsd3ServerProcessor::mkdir};
  handlers[folly::to_underlying(nfsv3Procs::symlink)] = {
      "SYMLINK", &Nfsd3ServerProcessor::symlink};
  handlers[folly::to_underlying(nfsv3Procs::mknod)] = {
      "MKNOD", &Nfsd3ServerProcessor::mknod};
  handlers[folly::to_underlying(nfsv3Procs::remove)] = {
      "REMOVE", &Nfsd3ServerProcessor::remove};
  handlers[folly::to_underlying(nfsv3Procs::rmdir)] = {
      "RMDIR", &Nfsd3ServerProcessor::rmdir};
  handlers[folly::to_underlying(nfsv3Procs::rename)] = {
      "RENAME", &Nfsd3ServerProcessor::rename};
  handlers[folly::to_underlying(nfsv3Procs::link)] = {
      "LINK", &Nfsd3ServerProcessor::link};
  handlers[folly::to_underlying(nfsv3Procs::readdir)] = {
      "READDIR", &Nfsd3ServerProcessor::readdir};
  handlers[folly::to_underlying(nfsv3Procs::readdirplus)] = {
      "READDIRPLUS", &Nfsd3ServerProcessor::readdirplus};
  handlers[folly::to_underlying(nfsv3Procs::fsstat)] = {
      "FSSTAT", &Nfsd3ServerProcessor::fsstat};
  handlers[folly::to_underlying(nfsv3Procs::fsinfo)] = {
      "FSINFO", &Nfsd3ServerProcessor::fsinfo};
  handlers[folly::to_underlying(nfsv3Procs::pathconf)] = {
      "PATHCONF", &Nfsd3ServerProcessor::pathconf};
  handlers[folly::to_underlying(nfsv3Procs::commit)] = {
      "COMMIT", &Nfsd3ServerProcessor::commit};

  return handlers;
}();

folly::Future<folly::Unit> Nfsd3ServerProcessor::dispatchRpc(
    folly::io::Cursor deser,
    folly::io::QueueAppender ser,
    uint32_t xid,
    uint32_t progNumber,
    uint32_t progVersion,
    uint32_t procNumber) {
  if (progNumber != kNfsdProgNumber) {
    serializeReply(ser, accept_stat::PROG_UNAVAIL, xid);
    return folly::unit;
  }

  if (progVersion != kNfsd3ProgVersion) {
    serializeReply(ser, accept_stat::PROG_MISMATCH, xid);
    XdrTrait<mismatch_info>::serialize(
        ser, mismatch_info{kNfsd3ProgVersion, kNfsd3ProgVersion});
    return folly::unit;
  }

  if (procNumber >= kNfs3dHandlers.size()) {
    XLOG(ERR) << "Invalid procedure: " << procNumber;
    serializeReply(ser, accept_stat::PROC_UNAVAIL, xid);
    return folly::unit;
  }

  auto handlerEntry = kNfs3dHandlers[procNumber];
  // TODO(xavierd): log the arguments too.
  FB_LOGF(*straceLogger_, DBG7, "{}()", handlerEntry.name);
  return (this->*handlerEntry.handler)(std::move(deser), std::move(ser), xid);
}
} // namespace

Nfsd3::Nfsd3(
    bool registerWithRpcbind,
    folly::EventBase* evb,
    std::unique_ptr<NfsDispatcher> dispatcher,
    const folly::Logger* straceLogger,
    std::shared_ptr<ProcessNameCache> /*processNameCache*/,
    folly::Duration /*requestTimeout*/,
    Notifications* /*notifications*/,
    bool caseSensitive)
    : server_(
          std::make_shared<Nfsd3ServerProcessor>(
              std::move(dispatcher),
              straceLogger,
              caseSensitive),
          evb) {
  if (registerWithRpcbind) {
    server_.registerService(kNfsdProgNumber, kNfsd3ProgVersion);
  }
}

Nfsd3::~Nfsd3() {
  // TODO(xavierd): wait for the pending requests, and the sockets being tore
  // down
  stopPromise_.setValue(Nfsd3::StopData{});
}

folly::SemiFuture<Nfsd3::StopData> Nfsd3::getStopFuture() {
  return stopPromise_.getSemiFuture();
}

} // namespace facebook::eden

#endif
