#pragma once

#include "ErrorCodes.h"

#include <vector>
#include <string>

#include <folly/RWSpinLock.h>
#include <folly/Memory.h>

namespace facebook {
namespace wdt {

const double kMbToB = 1024 * 1024;

// TODO: make those non copyable (or at least not copied by accident)
/// class representing statistics related to file transfer
class TransferStats {
 private:
  /// number of header bytes transferred
  size_t headerBytes_ = 0;
  /// number of data bytes transferred
  size_t dataBytes_ = 0;

  /// number of header bytes transferred as part of successful file transfer
  size_t effectiveHeaderBytes_ = 0;
  /// number of data bytes transferred as part of successful file transfer
  size_t effectiveDataBytes_ = 0;

  /// number of files successfully transferred
  size_t numFiles_ = 0;

  /// number of blocks successfully transferred
  size_t numBlocks_ = 0;

  /// number of failed transfers
  size_t failedAttempts_ = 0;

  /// status of the transfer
  ErrorCode errCode_ = OK;

  /// status of the remote
  ErrorCode remoteErrCode_ = OK;

  /// id of the owner object
  std::string id_;

  /// mutex to support synchronized access
  std::unique_ptr<folly::RWSpinLock> mutex_{nullptr};

 public:
  // making the object noncopyable
  TransferStats(const TransferStats &stats) = delete;
  TransferStats &operator=(const TransferStats &stats) = delete;
  TransferStats(TransferStats &&stats) = default;
  TransferStats &operator=(TransferStats &&stats) = default;

  explicit TransferStats(bool isLocked = false) {
    if (isLocked) {
      mutex_ = folly::make_unique<folly::RWSpinLock>();
    }
  }

  explicit TransferStats(const std::string &id, bool isLocked = false)
      : TransferStats(isLocked) {
    id_ = id;
  }

  void reset() {
    folly::RWSpinLock::WriteHolder lock(mutex_.get());
    headerBytes_ = dataBytes_ = 0;
    effectiveHeaderBytes_ = effectiveDataBytes_ = 0;
    numFiles_ = numBlocks_ = 0;
    failedAttempts_ = 0;
    errCode_ = remoteErrCode_ = OK;
  }

  /// @return number of header bytes transferred
  size_t getHeaderBytes() const {
    folly::RWSpinLock::ReadHolder lock(mutex_.get());
    return headerBytes_;
  }

  /// @return number of data bytes transferred
  size_t getDataBytes() const {
    folly::RWSpinLock::ReadHolder lock(mutex_.get());
    return dataBytes_;
  }

  /**
   * @param needLocking     specifies whether we need to lock or not. this is
   *                        for performance improvement. in sender, we do not
   *                        need locking for this call, even though the other
   *                        calls have to be locked
   *
   * @return                number of total bytes transferred
   */
  size_t getTotalBytes(bool needLocking = true) const {
    if (needLocking) {
      folly::RWSpinLock::ReadHolder lock(mutex_.get());
      return headerBytes_ + dataBytes_;
    }
    return headerBytes_ + dataBytes_;
  }

  /**
   * @return    number of header bytes transferred as part of successful file
   *            transfer
   */
  size_t getEffectiveHeaderBytes() const {
    folly::RWSpinLock::ReadHolder lock(mutex_.get());
    return effectiveHeaderBytes_;
  }

  /**
   * @return    number of data bytes transferred as part of successful file
   *            transfer
   */
  size_t getEffectiveDataBytes() const {
    folly::RWSpinLock::ReadHolder lock(mutex_.get());
    return effectiveDataBytes_;
  }

  /**
   * @return    number of total bytes transferred as part of successful file
   *            transfer
   */
  size_t getEffectiveTotalBytes() const {
    folly::RWSpinLock::ReadHolder lock(mutex_.get());
    return effectiveHeaderBytes_ + effectiveDataBytes_;
  }

  /// @return number of files successfully transferred
  size_t getNumFiles() const {
    folly::RWSpinLock::ReadHolder lock(mutex_.get());
    return numFiles_;
  }

  /// @return number of blocks successfully transferred
  size_t getNumBlocks() const {
    folly::RWSpinLock::ReadHolder lock(mutex_.get());
    return numBlocks_;
  }

  /// @return number of failed transfers
  size_t getFailedAttempts() const {
    folly::RWSpinLock::ReadHolder lock(mutex_.get());
    return failedAttempts_;
  }

  /// @return error code based on combinator of local and remote error
  ErrorCode getCombinedErrorCode() const {
    folly::RWSpinLock::ReadHolder lock(mutex_.get());
    if (errCode_ != OK || remoteErrCode_ != OK) {
      return ERROR;
    }
    return OK;
  }

  /// @return status of the transfer
  ErrorCode getErrorCode() const {
    folly::RWSpinLock::ReadHolder lock(mutex_.get());
    return errCode_;
  }

  /// @return status of the transfer on the remote end
  ErrorCode getRemoteErrorCode() const {
    folly::RWSpinLock::ReadHolder lock(mutex_.get());
    return remoteErrCode_;
  }

  const std::string &getId() const {
    folly::RWSpinLock::ReadHolder lock(mutex_.get());
    return id_;
  }

  /// @param number of additional data bytes transferred
  void addDataBytes(size_t count) {
    folly::RWSpinLock::WriteHolder lock(mutex_.get());
    dataBytes_ += count;
  }

  /// @param number of additional header bytes transferred
  void addHeaderBytes(size_t count) {
    folly::RWSpinLock::WriteHolder lock(mutex_.get());
    headerBytes_ += count;
  }

  /// one more file transfer failed
  void incrFailedAttempts() {
    folly::RWSpinLock::WriteHolder lock(mutex_.get());
    failedAttempts_++;
  }

  /// @param status of the transfer
  void setErrorCode(ErrorCode errCode) {
    folly::RWSpinLock::WriteHolder lock(mutex_.get());
    errCode_ = errCode;
  }

  /// @param status of the transfer on the remote end
  void setRemoteErrorCode(ErrorCode remoteErrCode) {
    folly::RWSpinLock::WriteHolder lock(mutex_.get());
    remoteErrCode_ = remoteErrCode;
  }

  /// @param id of the corresponding entity
  void setId(const std::string &id) {
    folly::RWSpinLock::WriteHolder lock(mutex_.get());
    id_ = id;
  }

  /// @param numFiles number of files successfully send
  void setNumFiles(size_t numFiles) {
    folly::RWSpinLock::WriteHolder lock(mutex_.get());
    numFiles_ = numFiles;
  }

  /// one more block successfully transferred
  void incrNumBlocks() {
    folly::RWSpinLock::WriteHolder lock(mutex_.get());
    numBlocks_++;
  }

  void decrNumBlocks() {
    folly::RWSpinLock::WriteHolder lock(mutex_.get());
    numBlocks_--;
  }

  /**
   * @param headerBytes header bytes transfered part of a successful file
   *                    transfer
   * @param dataBytes   data bytes transferred part of a successful file
   *                    transfer
   */
  void addEffectiveBytes(size_t headerBytes, size_t dataBytes) {
    folly::RWSpinLock::WriteHolder lock(mutex_.get());
    effectiveHeaderBytes_ += headerBytes;
    effectiveDataBytes_ += dataBytes;
  }

  void subtractEffectiveBytes(size_t headerBytes, size_t dataBytes) {
    folly::RWSpinLock::WriteHolder lock(mutex_.get());
    effectiveHeaderBytes_ -= headerBytes;
    effectiveDataBytes_ -= dataBytes;
  }

  TransferStats &operator+=(const TransferStats &stats);

  friend std::ostream &operator<<(std::ostream &os, const TransferStats &stats);
};

/**
 * class representing entire client transfer report
 */
class TransferReport {
 public:
  /**
   * This constructor moves all the stat objects to member variables. This is
   * only called at the end of transfer.
   */
  TransferReport(std::vector<TransferStats> &transferredSourceStats,
                 std::vector<TransferStats> &failedSourceStats,
                 std::vector<TransferStats> &threadStats,
                 std::vector<std::string> &failedDirectories, double totalTime,
                 size_t totalFileSize, size_t numDiscoveredFiles);

  /**
   * This function does not move the thread stats passed to it. This is called
   * by the progress reporter thread.
   */
  TransferReport(const std::vector<TransferStats> &threadStats,
                 double totalTime, size_t totalFileSize);

  /// constructor used by receiver, does move the thread stats
  explicit TransferReport(std::vector<TransferStats> &threadStats);
  /// @return   summary of the report
  const TransferStats &getSummary() const {
    return summary_;
  }
  /// @return   transfer throughput in Mbytes/sec
  double getThroughputMBps() const {
    return summary_.getEffectiveTotalBytes() / totalTime_ / kMbToB;
  }
  /// @return total time taken in transfer
  double getTotalTime() const {
    return totalTime_;
  }
  /// @return   stats for successfully transferred sources
  const std::vector<TransferStats> &getTransferredSourceStats() const {
    return transferredSourceStats_;
  }
  /// @return   stats for failed sources
  const std::vector<TransferStats> &getFailedSourceStats() const {
    return failedSourceStats_;
  }
  /// @return   stats for threads
  const std::vector<TransferStats> &getThreadStats() const {
    return threadStats_;
  }
  const std::vector<std::string> &getFailedDirectories() const {
    return failedDirectories_;
  }
  size_t getTotalFileSize() const {
    return totalFileSize_;
  }
  /// @param stats  stats to added
  void addTransferStats(const TransferStats &stats) {
    summary_ += stats;
  }
  void setErrorCode(ErrorCode errCode) {
    summary_.setErrorCode(errCode);
  }
  friend std::ostream &operator<<(std::ostream &os,
                                  const TransferReport &report);

 private:
  TransferStats summary_;
  /// stats for successfully transferred sources
  std::vector<TransferStats> transferredSourceStats_;
  /// stats for failed sources
  std::vector<TransferStats> failedSourceStats_;
  /// stats for client threads
  std::vector<TransferStats> threadStats_;
  /// directories which could not be opened
  std::vector<std::string> failedDirectories_;
  /// total transfer time
  double totalTime_{0};
  /// sum of all the file sizes
  size_t totalFileSize_{0};
};

/**
 * This class represents interface and default implementation of progress
 * reporting
 */
class ProgressReporter {
 public:
  /// this method is called before the transfer starts
  virtual void start() {
  }

  /**
   * This method gets called repeatedly with interval defined by
   * progress_report_interval. By default it displays transfer progress in
   * stdout. Example output [===>    ] 30% 5.00 MBytes/sec
   *
   * @param report                current transfer report
   */
  virtual void progress(const std::unique_ptr<TransferReport> &report);

  /**
   * This method gets called after the transfer ends
   *
   * @param report                final transfer report
   */
  virtual void end(const std::unique_ptr<TransferReport> &report);

  virtual ~ProgressReporter() {
  }

 private:
  /**
   * Displays progress of the transfer in stdout
   *
   * @param progress              progress percentage
   * @param throughput            current throughput
   *
   */
  void displayProgress(int progress, double throughput);
};
}
}
