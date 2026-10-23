//
// Created by Zach Lee on 2023/5/28.
//

#include <mtl/Queue.h>

namespace sky::mtl {

    Queue::~Queue()
    {
        if (queue) {
            [queue release];
            queue = nil;
        }
    }

    rhi::TransferTaskHandle Queue::UploadImage(const rhi::ImagePtr &image, const std::vector<rhi::ImageUploadRequest> &requests)
    {
        return CreateTask([]() {

            });
    }

    rhi::TransferTaskHandle Queue::UploadBuffer(const rhi::BufferPtr &image, const std::vector<rhi::BufferUploadRequest> &requests)
    {
        return CreateTask([]() {

        });
    }

    rhi::TransferTaskHandle Queue::ReadImage(const rhi::ImagePtr &image, rhi::ReadCallBack &&callback)
    {
        // TODO: 实现 Metal 读回（blit 到 shared buffer 后回调），目前仅占位
        return CreateTask([callback = std::move(callback)]() {
            callback(nullptr, 0, 0);
        });
    }

} // namespace sky::mtl

