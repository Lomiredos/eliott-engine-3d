#pragma once


namespace ee {

    template <class... Ts> //template varidique
    struct overloaded : Ts...{ // on herite de tous les truc passer dans la template
        using Ts::operator()...; //ca doit etre des lambda ou des truc qui on une surcharge de l'operator()
    };

    //donc on vient regrouper plusieur lambda dans une struct en gros

} //namespace ee