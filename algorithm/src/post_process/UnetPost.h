#pragma once

#include "../PostProcess.h"

namespace sophon_stream{
namespace algorithm {
namespace post_process {

class UnetPost : public algorithm::PostProcess {
    public:
        void init(algorithm::Context& context) override;

        void postProcess(algorithm::Context& context,
                        common::ObjectMetadatas & objectMetadatas) override;
};
}
}
}