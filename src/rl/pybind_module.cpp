#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "src/rl/STSEnv.h"

namespace py = pybind11;

PYBIND11_MODULE(sts_env, m) {
    m.doc() = "STS_CPP RL training environment";

    py::class_<StepResult>(m, "StepResult")
        .def_readonly("observation", &StepResult::observation)
        .def_readonly("reward", &StepResult::reward)
        .def_readonly("done", &StepResult::done)
        .def_readonly("info", &StepResult::info);

    py::class_<STSEnv>(m, "STSEnv")
        .def(py::init<int>(), py::arg("seed") = 0)
        .def("reset", &STSEnv::reset)
        .def("step", &STSEnv::step)
        .def("get_observation", &STSEnv::getObservation)
        .def("get_legal_action_mask", &STSEnv::getLegalActionMask)
        .def("action_space_size", &STSEnv::getActionSpaceSize)
        .def("observation_space_size", &STSEnv::getObservationSpaceSize);
}
