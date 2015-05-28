#include <ndn-cxx/management/nfd-controller.hpp>
#include <ndn-cxx/management/nfd-control-response.hpp>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>

namespace ndn {
namespace remote_register_prefix {

namespace po = boost::program_options;

static void
usage(std::ostream& os, const po::options_description& options)
{
  os << "Usage: register-prefix -p /laptop-prefix -f udp4://192.0.2.1:6363 -i /identity\n"
        "\n"
        "Send a remote prefix registration command.\n"
        "\n"
     << options;
}

class Options
{
public:
  Options()
    : commandPrefix("/localhop/nfd")
    , hasIdentity(false)
  {
  }

public:
  Name prefix;
  Name commandPrefix;
  bool hasIdentity;
  Name identity;
  std::string faceUri;
};

class Program
{
public:
  explicit
  Program(const Options& options)
    : m_controller(m_face, m_keyChain)
    , m_options(options)
    , m_faceId(0)
    , m_isOk(false)
  {
  }

  void
  run()
  {
    this->createFace();
    m_face.processEvents();
  }

private:
  void
  createFace()
  {
    nfd::ControlParameters params;
    params.setUri(m_options.faceUri);

    m_controller.start<nfd::FaceCreateCommand>(
      params,
      [this] (const nfd::ControlParameters& res) {
        this->m_faceId = res.getFaceId();
        this->enableNextHopFaceId();
      },
      [] (uint32_t code, const std::string& reason) {
        std::cerr << "createFace: " << code << " " << reason << std::endl;
        exit(1);
      });
  }

  void
  enableNextHopFaceId()
  {
    nfd::ControlParameters params;
    params.setLocalControlFeature(nfd::LOCAL_CONTROL_FEATURE_NEXT_HOP_FACE_ID);

    m_controller.start<nfd::FaceEnableLocalControlCommand>(
      params,
      bind([this] { this->prepareCommandInterest(); }),
      [] (uint32_t code, const std::string& reason) {
        std::cerr << "enableNextHopFaceId: " << code << " " << reason << std::endl;
        exit(1);
      });
  }

  void
  prepareCommandInterest()
  {
    nfd::ControlParameters params;
    params.setName(m_options.prefix);

    m_commandInterest = make_shared<Interest>(
                          nfd::RibRegisterCommand().getRequestName(
                            m_options.commandPrefix, params));

    if (m_options.hasIdentity) {
      m_keyChain.signByIdentity(*m_commandInterest, m_options.identity);
    }
    else {
      m_keyChain.sign(*m_commandInterest);
    }
    m_commandInterest->setNextHopFaceId(m_faceId);

    this->setClientControlStrategy();
  }

  void
  setClientControlStrategy()
  {
    nfd::ControlParameters params;
    params.setName(m_commandInterest->getName());
    params.setStrategy("ndn:/localhost/nfd/strategy/client-control");

    m_controller.start<nfd::StrategyChoiceSetCommand>(
      params,
      bind([this] { this->sendCommandInterest(); }),
      [] (uint32_t code, const std::string& reason) {
        std::cerr << "setClientControlStrategy: " << code << " " << reason << std::endl;
        exit(1);
      });
  }

  void
  sendCommandInterest()
  {
    m_face.expressInterest(*m_commandInterest,
      [this] (const Interest&, const Data& data) {
        nfd::ControlResponse resp(data.getContent().blockFromValue());
        if (resp.getCode() == 200) {
          std::cerr << "OK: " << m_options.prefix << " " << m_options.faceUri << std::endl;
          m_isOk = true;
        }
        else {
          std::cerr << "sendCommandInterest: " << resp.getCode() << " " << resp.getText() << std::endl;
          m_isOk = false;
        }
      },
      bind([this] {
        std::cerr << "sendCommandInterest: timeout" << std::endl;
        m_isOk = false;
        this->unsetClientControlStrategy();
      }));
  }

  void
  unsetClientControlStrategy()
  {
    nfd::ControlParameters params;
    params.setName(m_commandInterest->getName());

    m_controller.start<nfd::StrategyChoiceUnsetCommand>(
      params,
      bind([this] { exit(m_isOk ? 0 : 1); }),
      [] (uint32_t code, const std::string& reason) {
        std::cerr << "unsetClientControlStrategy: " << code << " " << reason << std::endl;
        exit(1);
      });
  }

private:
  Face m_face;
  KeyChain m_keyChain;
  nfd::Controller m_controller;

  const Options m_options;
  uint64_t m_faceId;
  shared_ptr<Interest> m_commandInterest;

  bool m_isOk;
};

int
main(int argc, char** argv)
{
  Options opt;

  po::options_description options("Required options");
  options.add_options()
    ("help,h", "print help message")
    ("prefix,p", po::value<Name>(&opt.prefix)->required(), "prefix")
    ("command,c", po::value<Name>(&opt.commandPrefix), "command prefix")
    ("face,f", po::value<std::string>(&opt.faceUri)->required(), "canonical FaceUri of router")
    ("identity,i", po::value<Name>(&opt.identity), "signing identity")
    ;
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, options), vm);
  try {
    po::notify(vm);
  }
  catch (boost::program_options::error&) {
    usage(std::cerr, options);
    return 2;
  }
  if (vm.count("help") > 0) {
    usage(std::cout, options);
    return 0;
  }
  opt.hasIdentity = vm.count("identity") > 0;

  Program program(opt);
  program.run();

  return 0;
}

} // namespace remote_register_prefix
} // namespace ndn

int
main(int argc, char** argv)
{
  return ndn::remote_register_prefix::main(argc, argv);
}