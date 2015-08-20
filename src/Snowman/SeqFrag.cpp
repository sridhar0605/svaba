#include "SeqFrag.h"

#include <iostream>

void SeqFrag::addScrambledEnds(size_t left_len, size_t right_len) {

  std::vector<std::string> TCGA = {"T","C","G","A"};
  
  std::string left, right;
  for (size_t i = 0; i < left_len; ++i) 
    left += TCGA[rand() % 4];
  for (size_t i = 0; i < right_len; ++i) 
    right += TCGA[rand() % 4];
  
  left_scramble = left;
  right_scramble = right;

  m_seq = left + m_seq + right;

}

void SeqFrag::addDels(size_t n, faidx_t * findex) {


  if (m_gr.strand == '-') // flip it back to process, then flip at end
    SnowTools::rcomplement(m_seq);

  if (n == 0 || m_seq.length() < 200)
    return;

  size_t spacing = m_seq.length() / (n+1);
  spacing = std::max((size_t)100, spacing);
  std::vector<int> breaks;
  for (size_t i = spacing; i < m_seq.length() - 100; i += spacing) {
    breaks.push_back(i);
  }  
  breaks.push_back(m_seq.length() - 1);

  if (spacing <= 30) {
    std::cerr << "Simulated indel spacing < 30. Should be > 30, reduce number of indels" << std::endl;
    exit(EXIT_FAILURE);
  }

  if (breaks.size() < 2)
    return;

  std::vector<std::string> TCGA = {"T","C","G","A"};

  // make a read sim to get random sized del
  ReadSim rs;

  // set up for the loop
  int del_cumsum = 0;
  int ins_cumsum = 0;
  std::vector<std::string> frags;
  frags.push_back(m_seq.substr(0, spacing));

  for (size_t i = 0; i < breaks.size() - 1; ++i) {

    // random size for events
    int ds = rs.getRandomIndelSize();
    
    char etype = (rand() % 2 ? 'D' : 'I');
    //char etype = 'D';

    // get the bounds
    int start = etype == 'D' ? breaks[i] + ds : breaks[i];
    int end = breaks[i+1];

    std::string ins_string = "";
    if (etype == 'I')
      for (int i = 0; i < ds; ++i) 
	ins_string += TCGA[rand() % 4];
    
    std::string lead_base = m_seq.substr(breaks[i]-1,1);
    std::string ref_seq = (etype == 'D' ? m_seq.substr(breaks[i], ds) : "");
    std::string alt_seq = (etype == 'D' ? "" : ins_string);

    // make the indel object
    Indel ind(ds, etype, ref_seq, alt_seq, lead_base);
    if (etype == 'D')
      ind.gr = SnowTools::GenomicRegion(m_gr.chr, m_gr.pos1 + breaks[i], m_gr.pos1 + breaks[i] + ds);
    else
      ind.gr = SnowTools::GenomicRegion(m_gr.chr, m_gr.pos1 + breaks[i], m_gr.pos1 + breaks[i] + 1);
    ind.frag_id = frag_id;

    // check that it's not on a blank region
    if (breaks[i] < 101 || m_seq.substr(breaks[i] - 100, 200).find("N") != std::string::npos) {
      start = breaks[i]; // don't put in the del or insert
      etype = 'D';
    } else {
      // store the indel and update reference counter
      m_indels.push_back(ind);
      if (etype == 'D')
	del_cumsum += ds;
      else
	ins_cumsum += ds;
    }

    // cut the sequence out
    if (etype == 'D') {
      frags.push_back(m_seq.substr(start, end - start));
    } else {
      // make the random insertion
      frags.push_back(ins_string + m_seq.substr(start, end - start));
    }

  }

  std::string new_seq;
  for (auto& i : frags) {
    new_seq += i;
  }

  //int len;
  //std::string chrstring = SnowTools::GenomicRegion::chrToString(m_gr.chr);
  //char * seq = faidx_fetch_seq(findex, const_cast<char*>(chrstring.c_str()), m_gr.pos2 - 1, m_gr.pos2 + del_cumsum - 1, &len);
  
  //if (!seq) {
  //  std::cerr << "Failed to get reference sequence at " << m_gr << std::endl;
  //  return;
  //}

  //new_seq += std::string(seq);

  if (m_gr.strand == '-') // flip back if need be
    SnowTools::rcomplement(new_seq);
  
  m_seq = new_seq;

}

void SeqFrag::getSeqFromRef(faidx_t * findex) {

  int len;
  std::string chrstring = SnowTools::GenomicRegion::chrToString(m_gr.chr);
  char * seq = faidx_fetch_seq(findex, const_cast<char*>(chrstring.c_str()), m_gr.pos1-1, m_gr.pos2-1, &len);
  
  if (!seq) {
    std::cerr << "Failed to get reference sequence at " << m_gr << std::endl;
    return;
  }
  
  m_seq = std::string(seq);

  // reverse complement if need
  if (m_gr.strand == '-')
    SnowTools::rcomplement(m_seq);
}

int SeqFrag::getLeftSide() const {

  if (m_gr.strand == '-')
    return m_gr.pos2;
  return m_gr.pos1;

}

int SeqFrag::getRightSide() const {

  if (m_gr.strand == '-')
    return m_gr.pos1;
  return m_gr.pos2;


}

std::ostream& operator<<(std::ostream& out, const SeqFrag& s) {
  
  out << s.m_gr;
  return out;
}
