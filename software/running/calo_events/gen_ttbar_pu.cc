// Pythia8 ttbar + exactly N pileup interactions, merged into one HepMC3 event.
// Matches software/analysis/strixel_ttbar/runtime.py: 14 TeV pp,
// Top:qqbar2ttbar=on hard scatter, SoftQCD:all=on pileup, Gaussian vertex
// sigma (0.0125, 0.0125, 55.5) mm and 5 ns per interaction.
//
// Usage: gen_ttbar_pu <nEvents> <seed> <nPileup> <output.hepmc3>

#include <HepMC3/GenEvent.h>
#include <HepMC3/GenParticle.h>
#include <HepMC3/GenVertex.h>
#include <HepMC3/WriterAscii.h>
#include <Pythia8/Pythia.h>

#include <cstdlib>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <tuple>

namespace {

constexpr double kVertexSigmaXY = 0.0125;            // mm
constexpr double kVertexSigmaZ = 55.5;               // mm
constexpr double kVertexSigmaT = 5.0 * 299.792458;   // 5 ns in mm/c
constexpr double kTau0MaxMm = 10.0;                  // longer-lived -> Geant4

void configure(Pythia8::Pythia& p, const std::string& process, int seed) {
  p.readString("Beams:idA = 2212");
  p.readString("Beams:idB = 2212");
  p.readString("Beams:eCM = 14000.");
  p.readString(process);
  p.readString("ParticleDecays:limitTau0 = on");
  p.readString("ParticleDecays:tau0Max = " + std::to_string(kTau0MaxMm));
  p.readString("Random:setSeed = on");
  p.readString("Random:seed = " + std::to_string(seed));
  p.readString("Next:numberCount = 0");
  p.readString("Print:quiet = on");
  if (!p.init()) {
    std::cerr << "Pythia init failed for " << process << "\n";
    std::exit(1);
  }
}

// Append final-state particles of one interaction, shifted to its vertex.
// Particles sharing a production point share one HepMC3 vertex.
void append(HepMC3::GenEvent& out, const Pythia8::Event& ev,
            const HepMC3::FourVector& shift) {
  std::map<std::tuple<double, double, double, double>, HepMC3::GenVertexPtr>
      vertices;
  for (int i = 0; i < ev.size(); ++i) {
    const auto& p = ev[i];
    if (!p.isFinal()) continue;
    auto key = std::make_tuple(p.xProd(), p.yProd(), p.zProd(), p.tProd());
    auto& v = vertices[key];
    if (!v) {
      v = std::make_shared<HepMC3::GenVertex>(HepMC3::FourVector(
          shift.x() + p.xProd(), shift.y() + p.yProd(),
          shift.z() + p.zProd(), shift.t() + p.tProd()));
      out.add_vertex(v);
    }
    auto gp = std::make_shared<HepMC3::GenParticle>(
        HepMC3::FourVector(p.px(), p.py(), p.pz(), p.e()), p.id(), 1);
    gp->set_generated_mass(p.m());
    v->add_particle_out(gp);
  }
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 5) {
    std::cerr << "usage: gen_ttbar_pu <nEvents> <seed> <nPileup> <out.hepmc3>\n";
    return 1;
  }
  const int nEvents = std::stoi(argv[1]);
  const int seed = std::stoi(argv[2]);
  const int nPileup = std::stoi(argv[3]);

  Pythia8::Pythia hard("", false), pileup("", false);
  configure(hard, "Top:qqbar2ttbar = on", seed);
  configure(pileup, "SoftQCD:all = on", seed + 1000000);

  std::mt19937_64 rng(seed);
  std::normal_distribution<double> gxy(0, kVertexSigmaXY), gz(0, kVertexSigmaZ),
      gt(0, kVertexSigmaT);
  auto vertex = [&] { return HepMC3::FourVector(gxy(rng), gxy(rng), gz(rng), gt(rng)); };

  HepMC3::WriterAscii writer(argv[4]);
  for (int iev = 0; iev < nEvents; ++iev) {
    HepMC3::GenEvent out(HepMC3::Units::GEV, HepMC3::Units::MM);
    out.set_event_number(iev);
    while (!hard.next()) {}
    append(out, hard.event, vertex());
    for (int ipu = 0; ipu < nPileup; ++ipu) {
      while (!pileup.next()) {}
      append(out, pileup.event, vertex());
    }
    writer.write_event(out);
    std::cout << "event " << iev << ": " << out.particles().size()
              << " final-state particles\n";
  }
  writer.close();
  return 0;
}
