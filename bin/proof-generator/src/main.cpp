//---------------------------------------------------------------------------//
// Copyright (c) 2018-2021 Mikhail Komarov <nemo@nil.foundation>
// Copyright (c) 2022 Aleksei Moskvin <alalmoskvin@nil.foundation>
// Copyright (c) 2022 Ilia Shirobokov <i.shirobokov@nil.foundation>
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//---------------------------------------------------------------------------//

#include <iostream>
#include <chrono>

#define BOOST_APPLICATION_FEATURE_NS_SELECT_BOOST

#include <boost/application.hpp>

#undef B0

#include <boost/random.hpp>
#include <boost/random/random_device.hpp>
#include <boost/json/src.hpp>
#include <boost/optional.hpp>
#include <boost/program_options/parsers.hpp>

#include <nil/proof-generator/aspects/args.hpp>
#include <nil/proof-generator/aspects/path.hpp>
#include <nil/proof-generator/aspects/configuration.hpp>
#include <nil/proof-generator/aspects/prover_vanilla.hpp>
#include <nil/proof-generator/detail/configurable.hpp>
#include <nil/proof-generator/prover.hpp>

template<typename F, typename First, typename... Rest>
inline void insert_aspect(F f, First first, Rest... rest) {
    f(first);
    insert_aspect(f, rest...);
}

template<typename F>
inline void insert_aspect(F f) {
}

template<typename Application, typename... Aspects>
inline bool insert_aspects(boost::application::context &ctx, Application &app, Aspects... args) {
    insert_aspect([&](auto aspect) { ctx.insert<typename decltype(aspect)::element_type>(aspect); }, args...);
    ::boost::shared_ptr<nil::proof_generator::aspects::path> path_aspect =
        boost::make_shared<nil::proof_generator::aspects::path>();

    ctx.insert<nil::proof_generator::aspects::path>(path_aspect);
    ctx.insert<nil::proof_generator::aspects::configuration>(
        boost::make_shared<nil::proof_generator::aspects::configuration>(path_aspect));
    ctx.insert<nil::proof_generator::aspects::prover_vanilla>(
        boost::make_shared<nil::proof_generator::aspects::prover_vanilla>(path_aspect));

    return true;
}

template<typename Application>
inline bool configure_aspects(boost::application::context &ctx, Application &app) {
    typedef nil::proof_generator::detail::configurable<nil::dbms::plugin::variables_map,
                                                       nil::dbms::plugin::cli_options_description,
                                                       nil::dbms::plugin::cfg_options_description>
        configurable_aspect_type;

    boost::strict_lock<boost::application::aspect_map> guard(ctx);
    boost::shared_ptr<nil::proof_generator::aspects::args> args = ctx.find<nil::proof_generator::aspects::args>(guard);
    boost::shared_ptr<nil::proof_generator::aspects::configuration> cfg =
        ctx.find<nil::proof_generator::aspects::configuration>(guard);

    for (boost::shared_ptr<void> itr : ctx) {
        boost::static_pointer_cast<configurable_aspect_type>(itr)->set_options(cfg->cli());
        // boost::static_pointer_cast<configurable_aspect_type>(itr)->set_options(cfg->cfg());
    }

    try {
        boost::program_options::store(
            boost::program_options::parse_command_line(args->argc(), args->argv(), cfg->cli()), cfg->vm());
    } catch (const std::exception &e) {
        std::cout << e.what() << std::endl;
    }

    for (boost::shared_ptr<void> itr : ctx) {
        boost::static_pointer_cast<configurable_aspect_type>(itr)->initialize(cfg->vm());
    }

    return false;
}

struct prover {
    prover(boost::application::context &context) : context_(context) {
    }

    template <typename BlueprintFieldType>
    int run_prover() {
        auto prover_task = [&] {
            return nil::proof_generator::prover<BlueprintFieldType>(circuit_file_path, assignment_file_path, proof_file, skip_verification) ? 0 : 1;
        };
        return prover_task();
    }

    int operator()() {
        BOOST_APPLICATION_FEATURE_SELECT
        circuit_file_path = context_.find<nil::proof_generator::aspects::prover_vanilla>()->input_circuit_file_path();
        assignment_file_path =
            context_.find<nil::proof_generator::aspects::prover_vanilla>()->input_assignment_file_path();

        skip_verification = context_.find<nil::proof_generator::aspects::prover_vanilla>()->is_skip_verification_mode_on();

        proof_file = context_.find<nil::proof_generator::aspects::prover_vanilla>()->output_proof_file_path();

        nil::proof_generator::detail::CurveType curve_type = context_.find<nil::proof_generator::aspects::prover_vanilla>()->curve_type();
        switch (curve_type) {
            case nil::proof_generator::detail::PALLAS: {
                using curve_type = nil::crypto3::algebra::curves::pallas;
                using BlueprintFieldType = typename curve_type::base_field_type;
                return run_prover<BlueprintFieldType>();
            }
            case nil::proof_generator::detail::VESTA: {
                BOOST_LOG_TRIVIAL(error) << "vesta curve based circuits are not supported yet";
                return 1;
            }
            case nil::proof_generator::detail::ED25519: {
                BOOST_LOG_TRIVIAL(error) << "ed25519 curve based circuits are not supported yet";
                return 1;
            }
            case nil::proof_generator::detail::BLS12381: {
                BOOST_LOG_TRIVIAL(error) << "bls12-381 curve based circuits proving is temporarily disabled";
                return 1;
            }
        };
    }

    boost::filesystem::path proof_file;
    boost::filesystem::path circuit_file_path;
    boost::filesystem::path assignment_file_path;
    bool skip_verification;

    boost::application::context &context_;
};

bool setup(boost::application::context &context) {
    return false;
}

int main(int argc, char *argv[]) {
    boost::system::error_code ec;
    /*<<Create a global context application aspect pool>>*/
    boost::application::context ctx;

    boost::application::auto_handler<prover> app(ctx);

    if (!insert_aspects(ctx, app, boost::make_shared<nil::proof_generator::aspects::args>(argc, argv))) {
        std::cout << "[E] Application aspects configuration failed!" << std::endl;
        return 1;
    }
    if (configure_aspects(ctx, app)) {
        std::cout << "[I] Setup changed the current configuration." << std::endl;
    }
    // my server instantiation
    int result = boost::application::launch<boost::application::common>(app, ctx, ec);

    if (ec) {
        std::cout << "[E] " << ec.message() << " <" << ec.value() << "> " << std::endl;
    }

    return result;
}
