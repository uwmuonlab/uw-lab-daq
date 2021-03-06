#include "async_root_writer.hh"

namespace sc {

AsyncRootWriter::AsyncRootWriter()
{
  pf_ = new TFile("test.root", "recreate");

  UpdateTime();
}

AsyncRootWriter::AsyncRootWriter(string filename)
{
  pf_ = new TFile(filename.c_str(), "recreate");

  UpdateTime();
}

AsyncRootWriter::~AsyncRootWriter()
{
  // Take care of root stuff.
  pf_->Close();

  // Take are of data.
//  for (auto it = data_map_vec_.begin(); it != data_map_vec_.end(); ++it) {
//    for (auto it_2 = (*it).begin(); it_2 != (*it).end(); ++it_2) {
//      delete it_2->second;
//    }
//  }

//  delete time_info_;
//  delete pf_;
}

void AsyncRootWriter::CreateTree(const string &setup)
{
  string str; // used for utility

  // Turn the setup string into an easily parseable stream.
  std::istringstream ss(setup);

  // Now get the first line which is the device key.
  std::getline(ss, str, ':');
  string dev_key(str);

  // Check if the device key is already in use, when messages backup this
  // needs to be accounted for.
  if (key_map_.find(str) != key_map_.end()) {
    return;
  }

  // The next line is the device name
  std::getline(ss, str, ':');

  // Check if it is already in use.
  if (name_map_.find(str) != name_map_.end()) {

    // The device name is taken.
    cout << "Device name " << str << " is already in use." << endl;

    // Find a new name.
    int count = 0;
    string dev_name(str);

    while (name_map_.find(dev_name) != name_map_.end()) {
      dev_name = str;
      dev_name.append("_");
      dev_name.append(std::to_string(count++));
    }

    str = dev_name;
    cout << "Using device name " << str << " instead." << endl;
  }

  // Add the tree to the tree name map.
  string tree_name = str;
  name_map_[tree_name] = pt_vec_.size();
  key_map_[dev_key] = pt_vec_.size();

  // Create the tree and grab a handy pointer to it.
  pt_vec_.push_back(new TTree(tree_name.c_str(), tree_name.c_str()));
  TTree *pt = pt_vec_[key_map_[dev_key]];
  pt->SetAutoFlush(30);

  std::map<string, double *> data_map;
  // Now create the branches.
  while (ss.good()) {

    // Get the next variable.
    std::getline(ss, str, ':');

    // Check if we are done.
    if (str.find("__EOM__") < str.size()) break;

    // Allocate as a double (low data volumes should be fine).
    double *pdata = new double;
    data_map[str] = pdata;

    // Create the branch
    string branch_name(str);
    string branch_vars(str);
    branch_vars.append("/D");

    pt->Branch(branch_name.c_str(), 
               data_map[branch_name], 
               branch_vars.c_str());
  }

  string time_str("sec/I:min/I:hour/I:mday/I:mon/I:year/I:wday/I:yday/I");
  pt->Branch("time", &time_info_, time_str.c_str());

  data_map_vec_.push_back(data_map);  
  cout << "Device " << tree_name << " tree created successfully." << endl;
}

int AsyncRootWriter::PushData(const string &data)
{
  // Convert to a stringstream
  std::istringstream ss(data);
  string str, val;

  // Get the tree name and index.
  std::getline(ss, str, ':');

  // Make sure the tree exists.
  if (key_map_.find(str) == key_map_.end()) return 1;

  // Else grab the tree's data map.
  int tree_idx = key_map_[str];
  std::map<string, double *> data_map = data_map_vec_[tree_idx];

  while (ss.good()) {

    // Get the variable/branch name.
    std::getline(ss, str, ':');

    // Check if we are done.
    if (str.find("__EOM__") < str.size()) break;

    // Get the data.
    std::getline(ss, val, ':');

    *data_map[str] = std::stod(val);
  }

  UpdateTime();
  pt_vec_[tree_idx]->Fill();
  pf_->Flush();

  return 0;
}

void AsyncRootWriter::WriteFile()
{
  pf_->Write();
}

void AsyncRootWriter::UpdateTime()
{
  tm *tm_info;
  time_t now;
  time(&now);
  tm_info = localtime(&now);

  time_info_.sec = tm_info->tm_sec;
  time_info_.min = tm_info->tm_min;
  time_info_.hour = tm_info->tm_hour;
  time_info_.mday = tm_info->tm_mday;
  time_info_.mon  = tm_info->tm_mon;
  time_info_.year = tm_info->tm_year;
  time_info_.wday = tm_info->tm_wday;
  time_info_.yday = tm_info->tm_yday;
}

} // ::sc
