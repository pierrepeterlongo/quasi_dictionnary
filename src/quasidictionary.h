#ifndef QUASIdictionary_H
#define QUASIdictionary_H


//#include "../BooPHF/BooPHF.h"
#include <iostream>
#include <gatb/gatb_core.hpp>
#include "native_bit_vector_array.h"
#include "probabilistic_set.h"
#include <mutex>

typedef boomphf::SingleHashFunctor<u_int64_t>  hasher_t;
typedef boomphf::mphf<  u_int64_t, hasher_t  > boophf_t;


static const int nbMutex(10000);
static mutex myMutex[nbMutex];



/**
 
 template <class T,class U>
 class MyIterator : public std::iterator<std::input_iterator_tag, int>
 {
 tuple<T,U>* _tuples ;
 public:
 MyIterator(tuple<T,U>* tuples) :_tuples(tuples) {}
 MyIterator(const MyIterator& mit) : _tuples(mit._tuples) {}
 MyIterator& operator++() {++_tuples;return *this;}
 MyIterator operator++(int) {MyIterator tmp(*this); operator++(); return tmp;}
 bool operator==(const MyIterator& rhs) {return _tuples==rhs._tuples;}
 bool operator!=(const MyIterator& rhs) {return _tuples!=rhs._tuples;}
 T& operator*() {return get<0>(*_tuples);}
 };
 
 
 */



    
    template<typename POD>
    std::ostream& serialize(std::ostream& os, std::vector<POD> const& v)
    {
        // this only works on built in data types (PODs)
        static_assert(std::is_trivial<POD>::value && std::is_standard_layout<POD>::value,
            "Can only serialize POD types with this function");

        auto size = v.size();
        os.write(reinterpret_cast<char const*>(&size), sizeof(size));
        os.write(reinterpret_cast<char const*>(v.data()), v.size() * sizeof(POD));
        return os;
    }

    template<typename POD>
    std::istream& deserialize(std::istream& is, std::vector<POD>& v)
    {
        static_assert(std::is_trivial<POD>::value && std::is_standard_layout<POD>::value,
            "Can only deserialize POD types with this function");

        decltype(v.size()) size;
        is.read(reinterpret_cast<char*>(&size), sizeof(size));
        v.resize(size);
        is.read(reinterpret_cast<char*>(v.data()), v.size() * sizeof(POD));
        return is;
    }


// iterator from disk file of T with buffered read
template <class T>
class bfile_iterator_first : public std::iterator<std::forward_iterator_tag, T>{
    
public:
    
    
    bfile_iterator_first() : _is(nullptr), _pos(0) ,_inbuff (0), _cptread(0){
        _buffsize = 10000;
        _buffer = (T *) malloc(_buffsize*sizeof(T));
    }
    
    
    bfile_iterator_first(const bfile_iterator_first& cr){
        _buffsize = cr._buffsize;
        _pos = cr._pos;
        _is = cr._is;
        _buffer = (T *) malloc(_buffsize*sizeof(T));
        memcpy(_buffer,cr._buffer,_buffsize*sizeof(T) );
        _inbuff = cr._inbuff;
        _cptread = cr._cptread;
        _elem = cr._elem;
    }
    
    
    bfile_iterator_first(FILE* is): _is(is) , _pos(0) ,_inbuff (0), _cptread(0){
        _buffsize = 10000;
        _buffer = (T *) malloc(_buffsize*sizeof(T));
        int reso = fseek(_is,0,SEEK_SET);
        advance();
    }
    
    
    ~bfile_iterator_first(){
        if(_buffer!=NULL)
            free(_buffer);
    }
    
    
    T const& operator*()  {  return _elem;  }
    
    
    bfile_iterator_first& operator++(){
        advance();
        return *this;
    }
    
    
    friend bool operator==(bfile_iterator_first const& lhs, bfile_iterator_first const& rhs){
        if (!lhs._is || !rhs._is)  {  if (!lhs._is && !rhs._is) {  return true; } else {  return false;  } }
        assert(lhs._is == rhs._is);
        return rhs._pos == lhs._pos;
    }
    
    
    friend bool operator!=(bfile_iterator_first const& lhs, bfile_iterator_first const& rhs)  {  return !(lhs == rhs);  }
    
    
private:
    void advance()
    {
        ++_pos;
        
        if(_cptread >= _inbuff){
            int res = fread(_buffer,sizeof(T),_buffsize,_is);
            _inbuff = res; _cptread = 0;
            
            if(res == 0){
                _is = nullptr;
                _pos = 0;
                return;
            }
        }
        
        _elem = _buffer[_cptread];
        ++_cptread;
        ++_cptread;
    }
    T _elem;
    FILE * _is;
    unsigned long _pos;
    
    T * _buffer; // for buffered read
    int _inbuff, _cptread;
    int _buffsize;
};

template <class T>
class file_binary_first{
    
public:
    
    
    file_binary_first(const char* filename){
        _is = fopen(filename, "rb");
        if (!_is) {
            throw std::invalid_argument("Error opening " + std::string(filename));
        }
    }
    
    
    ~file_binary_first(){
        fclose(_is);
    }
    
    
    bfile_iterator_first<T> begin() const{
        return bfile_iterator_first<T>(_is);
    }
    
    bfile_iterator_first<T> end() const {return bfile_iterator_first<T>(); }
    
    size_t size () const{
        return 0;
    }//todo ?
    
private:
    FILE * _is;
};


// iterator from disk file of T with buffered read
template <class T>
class bfile_iterator : public std::iterator<std::forward_iterator_tag, T>{
public:
    
    bfile_iterator()
    : _is(nullptr)
    , _pos(0) ,_inbuff (0), _cptread(0)
    {
        _buffsize = 10000;
        _buffer = (T *) malloc(_buffsize*sizeof(T));
    }
    
    bfile_iterator(const bfile_iterator& cr)
    {
        _buffsize = cr._buffsize;
        _pos = cr._pos;
        _is = cr._is;
        _buffer = (T *) malloc(_buffsize*sizeof(T));
        memcpy(_buffer,cr._buffer,_buffsize*sizeof(T) );
        _inbuff = cr._inbuff;
        _cptread = cr._cptread;
        _elem = cr._elem;
    }
    
    bfile_iterator(FILE* is): _is(is) , _pos(0) ,_inbuff (0), _cptread(0)
    {
        _buffsize = 10000;
        _buffer = (T *) malloc(_buffsize*sizeof(T));
        int reso = fseek(_is,0,SEEK_SET);
        advance();
    }
    
    ~bfile_iterator()
    {
        if(_buffer!=NULL)
            free(_buffer);
    }
    
    
    T const& operator*()  {  return _elem;  }
    
    bfile_iterator& operator++()
    {
        advance();
        return *this;
    }
    
    friend bool operator==(bfile_iterator const& lhs, bfile_iterator const& rhs)
    {
        if (!lhs._is || !rhs._is)  {  if (!lhs._is && !rhs._is) {  return true; } else {  return false;  } }
        assert(lhs._is == rhs._is);
        return rhs._pos == lhs._pos;
    }
    
    friend bool operator!=(bfile_iterator const& lhs, bfile_iterator const& rhs)  {  return !(lhs == rhs);  }
private:
    void advance()
    {
        ++_pos;
        
        if(_cptread >= _inbuff)
        {
            int res = fread(_buffer,sizeof(T),_buffsize,_is);
            _inbuff = res; _cptread = 0;
            
            if(res == 0)
            {
                _is = nullptr;
                _pos = 0;
                return;
            }
        }
        
        _elem = _buffer[_cptread];
        ++_cptread;
    }
    T _elem;
    FILE * _is;
    unsigned long _pos;
    
    T * _buffer; // for buffered read
    int _inbuff, _cptread;
    int _buffsize;
};




template <class T>
class file_binary{
public:
    
    file_binary(const char* filename)
    {
        _is = fopen(filename, "rb");
        if (!_is) {
            throw std::invalid_argument("Error opening " + std::string(filename));
        }
    }
    
    ~file_binary()
    {
        fclose(_is);
    }
    
    bfile_iterator<T> begin() const
    {
        return bfile_iterator<T>(_is);
    }
    
    bfile_iterator<T> end() const {return bfile_iterator<T>(); }
    
    size_t        size () const  {  return 0;  }//todo ?
    
private:
    FILE * _is;
};



template <typename Keys, typename Values>
class quasidictionary
{
public:
    
    bool contains(u_int64_t key){
        u_int64_t index = _bphf->lookup(key);                           // grab the index from bbhash
        if(index >= _nelement)  return false;                           // bbhash was able to detect the element as non indexed
        if(_fingerprint_size>0) return _prob_set->exists(index, key);   // check the fingerprint equality
        else                    return true;                            // The element exists
    }
    
    
    
    /**
     * @brief createMPHF constructs the MPHF from the set of keys
     */
    void createMPHF(){
        
        this->_bphf = new boomphf::mphf<u_int64_t,hasher_t>(this->_nelement,this->_itKeyOnly,this->_nthreads,this->_gammaFactor); // WARNING: this method does not work with n_threads==0
        
        
    }

    void save(std::ostream& os) const {
        os.write(reinterpret_cast<char const*>(&_nelement), sizeof(_nelement));
		os.write(reinterpret_cast<char const*>(&_gammaFactor), sizeof(_gammaFactor));
		os.write(reinterpret_cast<char const*>(&_nthreads), sizeof(_nthreads));
        os.write(reinterpret_cast<char const*>(&_fingerprint_size), sizeof(_fingerprint_size));
        _prob_set->save(os);
        _bphf->save(os);
    }

    void load(std::istream& is){
        is.read(reinterpret_cast<char *>(&_nelement), sizeof(_nelement));
		is.read(reinterpret_cast<char *>(&_gammaFactor), sizeof(_gammaFactor));
		is.read(reinterpret_cast<char *>(&_nthreads), sizeof(_nthreads));
		is.read(reinterpret_cast<char *>(&_fingerprint_size), sizeof(_fingerprint_size));
        _prob_set = new probabilisticSet();
        _prob_set->load(is);
        _bphf  = new boomphf::mphf<u_int64_t,hasher_t>();
        _bphf->load(is);
    }



protected:
    
    
    /**
     * @brief _nelement number of elements to be stored (fix)
     */
    u_int64_t _nelement;
    
    double _gammaFactor;
    

    int _nthreads;
    
    /**
     * @brief _fingerprint_size size of the fingerprint. In [0,61]
     */
    int _fingerprint_size;


    /**
     * @brief _prob_set probabilistic set used to inform about the existence of a query element.
     */
    probabilisticSet * _prob_set;
    
    
    
    
    /**
     * @brief _bphf MPHF used to assign an index to a key
     */
    boophf_t * _bphf;

    
    
    
    /**
     * @brief _itKeyOnly iterator on the key set
     * This is used only during MPHF creation. Not serialized nor loaded when reading a dumped quasi-dictionary
     */
    Keys _itKeyOnly;




};



template <typename Keys, typename Values>
class quasidictionaryKeyValue : public quasidictionary<Keys,Values>
{
public:
    // Creates a probabilisticSet for the set of elements.
    // Creates a MPHF for the elements
    quasidictionaryKeyValue(){}
    
    
    /**
     * @brief quasidictionary : probabilistic dictionary: may have false positives
     * @param nelement: number of elements to store
     * @param itKey: iterator over the keys to be stored
     * @param it: iterator over the keys and their values to store
     * @param fingerprint_size: size of the fingerprint associated to each key to verify its existance in the original set. Can be set to zero if this is not needed
     * @param value_size: size of each value associated to each key. This must be below 62 bits.
     * @param gammaFactor: for MPHF
     * @param nthreads: for MPHF construction
     */
    quasidictionaryKeyValue(u_int64_t nelement, Keys& itKey, Values& it, const int fingerprint_size, const int value_size,  int nthreads=0, double gammaFactor=2)
    {
        
        
        
        this->_itKeyOnly = itKey;
        _itKeyValue = it;
        _valueSize = value_size;
        this->_nelement = nelement;
        this->_gammaFactor = gammaFactor;
        this->_fingerprint_size = fingerprint_size;
        this->_nthreads = nthreads;
        if (this->_nthreads == 0) this->_nthreads=10; //  with this->_nthreads=0: seg fault after mpfh construction (mphf incompatible)
        
        
        
        cout << "NB elems: " << this->_nelement << " elems" << endl;
        cout << "Fingerprint size: " << this->_fingerprint_size << " bits" << endl;
        cout << "Value size: " << _valueSize << " bits" << endl;
        
        this->createMPHF();
        createValues();
        
        
    }
    
    
    
    /**
     * @brief get_value: returns a value from a key in a quasi dictionary
     * @param key: the key of the seek value
     * @param exists: set to true is detected as indexed in the quasidictionary, else false
     * @return 0 if nothing found (and exists set to false) or the value associated to the key else
     */
    u_int64_t get_value(const u_int64_t key, bool &exists)const{
        const u_int64_t& index = this->_bphf->lookup(key);
        
        if(index >= this->_nelement)                                                {exists = false; return 0;}   // detected non indexed by bbhash
        if (this->_fingerprint_size>0 and not this->_prob_set->exists(index, key))  {exists = false; return 0;}   // bad fingerprint
     
        exists = true;
        return this->_values.get_i(index);
    }
    
    
    
    
    /**
     * @brief createValues once the MPHF is constructed: construct the probabilisticSet storing the fingerprints and stores the values in a newly constructed bitArraySet
     */
    void createValues(){
        cout << "creating values" << endl;
        if(this->_fingerprint_size>0)
            this->_prob_set = new probabilisticSet(this->_nelement, this->_fingerprint_size);
        
        this->_values = bitArraySet(this->_nelement, this->_valueSize);
        
        for(auto& key_value: this->_itKeyValue){
            const u_int64_t& index = this->_bphf->lookup(std::get<0>(key_value));
            if (this->_fingerprint_size>0){
                this->_prob_set->add(index, std::get<0>(key_value));
            }
            this->_values.set_i(index, std::get<1>(key_value));
        }
    }
    
    
    void save(std::ostream& os){
        throw "save for quasidictionaryKeyValue is not implemented yet.";
    }

    
    void load(std::istream& is){
        throw "load for quasidictionaryKeyValue is not implemented yet.";
    }

private:
    
    
    /**
     * @brief _values stores for each indexed element the value associated to a key
     */
    bitArraySet& _values;
    
    /**
     * @brief _valueSize Size of the stored values. In [0,61] (but zero is really, really stupid.
     */
    int _valueSize;
    
    /**
     * @brief _itKeyValue iterator on tuples key,value
     */
    Values _itKeyValue;
};




template <typename Keys, typename ValuesType>
class quasidictionaryVectorKeyGeneric : public quasidictionary<Keys,ValuesType>
{
public:
    // Creates a probabilisticSet for the set of elements.
    // Creates a MPHF for the elements
    quasidictionaryVectorKeyGeneric(){
    }
    
    
    
    
    /**
     * @brief quasidictionary : probabilistic dictionary: may have false positives
     * @param nelement: number of elements to store
     * @param itKey: iterator over the keys to be stored
     * @param it: iterator over the keys and their values to store
     * @param fingerprint_size: size of the fingerprint associated to each key to verify its existance in the original set. Can be set to zero if this is not needed
     * @param value_size: size of each value associated to each key. This must be below 62 bits.
     * @param gammaFactor: for MPHF
     * @param nthreads: for MPHF construction
     */
    quasidictionaryVectorKeyGeneric(u_int64_t nelement, Keys& itKey, const int fingerprint_size, int nthreads=0, double gammaFactor=2)
    {
        this->_nelement = nelement;
        this->_itKeyOnly = itKey;
        this->_fingerprint_size = fingerprint_size;
        this->_gammaFactor = gammaFactor;
        this->_nthreads = nthreads;
        if (this->_nthreads == 0) this->_nthreads=10; // TODO: unknown bug with this->_nthreads=0: seg fault after mpfh construction.
        this->_values = std::vector< vector<ValuesType> > (this->_nelement);
        
        cout << "Quasidictionary: NB elems         =" << this->_nelement << " elems" << endl;
        cout << "Quasidictionary: Fingerprint size =" << this->_fingerprint_size << " bits" << endl;
        
        this->createMPHF();
        
        if (this->_fingerprint_size>0){
            this->_prob_set = new probabilisticSet(this->_nelement, this->_fingerprint_size);
            for(const auto& key: this->_itKeyOnly){
                const u_int64_t& index = this->_bphf->lookup(key);
                this->_prob_set->add(index, key);
            }
        }
    }
    
    
    bool set_value(u_int64_t key, ValuesType &value){
        const u_int64_t& index = this->_bphf->lookup(key);
        
        if(index >= this->_nelement)                                                return false;   // detected non indexed by bbhash
        if (this->_fingerprint_size>0 and not this->_prob_set->exists(index, key))  return false;   // bad fingerprint
        
        myMutex[index%nbMutex].lock();
        this->_values[index].push_back(value);
        myMutex[index%nbMutex].unlock();
        return true;
    }
    
    
    /**
     *
     */
    bool set_value(u_int64_t key, ValuesType &value, ISynchronizer* synchro){
        const u_int64_t& index = this->_bphf->lookup(key);
        
        if(index >= this->_nelement)                                                return false;   // detected non indexed by bbhash
        if (this->_fingerprint_size>0 and not this->_prob_set->exists(index, key))  return false;   // bad fingerprint
        
        synchro->lock();
        this->_values[index].push_back(value);
        synchro->unlock();
        return true;
    }
    
    
    /**
     * @brief get_value: returns a value from a key in a quasi dictionary
     * @param key: the key of the seek value
     * @param exists: set to true is detected as indexed in the quasidictionary, else false
     * @return 0 if nothing found (and exists set to false) or the value associated to the key else
     */
    void get_value(const u_int64_t key, bool &exists, vector<ValuesType>& value)const{
        const u_int64_t& index = this->_bphf->lookup(key);
        
        if(index >= this->_nelement)                                                {exists = false; return;}   // detected non indexed by bbhash
        if (this->_fingerprint_size>0 and not this->_prob_set->exists(index, key))  {exists = false; return;}   // bad fingerprint
        
        exists = true;
        value=this->_values[index];
    }
    
    

    void save(std::ostream& os) 
    {
        // save the qd
        quasidictionary<Keys,ValuesType>::save(os);
        // save the _values
        // save the whole vector
        auto size = _values.size();
        os.write(reinterpret_cast<char const*>(&size), sizeof(size));

        // save each subvector: 
        for (auto i=0; i<_values.size(); i++){
            serialize(os, _values[i]);
        }  


        
        
    }


    void load(std::istream& is) 
    {
        // read the qd
        quasidictionary<Keys,ValuesType>::load(is);

        // read the values
        _values = std::vector< std::vector<ValuesType> >() ;
        // read the whole vector
        decltype(_values.size()) size;
        is.read(reinterpret_cast<char*>(&size), sizeof(size));

        // read each subvector: 
        for (auto i=0; i<size; i++){
            std::vector<ValuesType> current_vector;
            deserialize(is, current_vector);
            _values.push_back(current_vector);
        }
    }
    
private:
    
    
    
    /**
     * @brief _values stores for each indexed element the value associated to a key
     */
    std::vector< vector<ValuesType> > _values;
    
    
};




template <typename Keys, typename ValuesType>
class quasidictionaryKeyGeneric : public quasidictionary<Keys,ValuesType>
{
public:
    // Creates a probabilisticSet for the set of elements.
    // Creates a MPHF for the elements
    quasidictionaryKeyGeneric(){
    }
    
    
    
    
    /**
     * @brief quasidictionary : probabilistic dictionary: may have false positives
     * @param nelement: number of elements to store
     * @param itKey: iterator over the keys to be stored
     * @param it: iterator over the keys and their values to store
     * @param fingerprint_size: size of the fingerprint associated to each key to verify its existance in the original set. Can be set to zero if this is not needed
     * @param value_size: size of each value associated to each key. This must be below 62 bits.
     * @param gammaFactor: for MPHF
     * @param nthreads: for MPHF construction
     */
    quasidictionaryKeyGeneric(u_int64_t nelement, Keys& itKey, const int fingerprint_size, int nthreads=0, double gammaFactor=2)
    {
        this->_nelement = nelement;
        this->_itKeyOnly = itKey;
        this->_fingerprint_size = fingerprint_size;
        this->_gammaFactor = gammaFactor;
        this->_nthreads = nthreads;
        if (this->_nthreads == 0) this->_nthreads=10; // TODO: unknown bug with this->_nthreads=0: seg fault after mpfh construction.
        this->_values = std::vector< ValuesType > (this->_nelement);
        
        cout << "NB elems: " << this->_nelement << " elems" << endl;
        cout << "Fingerprint size: " << this->_fingerprint_size << " bits" << endl;
        
        this->createMPHF();
        
        if (this->_fingerprint_size>0){
            this->_prob_set = new probabilisticSet(this->_nelement, this->_fingerprint_size);
            for(const auto& key: this->_itKeyOnly){
                const u_int64_t& index = this->_bphf->lookup(key);
                this->_prob_set->add(index, key);
            }
        }
    }
    
    
    bool set_value(u_int64_t key, ValuesType value){
        const u_int64_t& index = this->_bphf->lookup(key);
        if(index >= this->_nelement)                                                return false;   // detected non indexed by bbhash
        if (this->_fingerprint_size>0 and not this->_prob_set->exists(index, key))  return false;   // bad fingerprint
        
        myMutex[index%nbMutex].lock();
        this->_values[index] = value;
        myMutex[index%nbMutex].unlock();
        return true;
    }
    
    
    bool set_value(u_int64_t key, ValuesType value, ISynchronizer* synchro){
        const u_int64_t& index = this->_bphf->lookup(key);
        
        if(index >= this->_nelement)                                                return false;   // detected non indexed by bbhash
        if (this->_fingerprint_size>0 and not this->_prob_set->exists(index, key))  return false;   // bad fingerprint
        
        synchro->lock();
        this->_values[index] = value;
        synchro->unlock();
        return true;
    }
    
    
    /**
     * @brief get_value: returns a value from a key in a quasi dictionary
     * @param key: the key of the seek value
     * @param exists: set to true is detected as indexed in the quasidictionary, else false
     * @return 0 if nothing found (and exists set to false) or the value associated to the key else
     */
    void get_value(const u_int64_t key, bool &exists, ValuesType& value)const{
        const u_int64_t& index = this->_bphf->lookup(key);
        if(index >= this->_nelement)                                                {exists = false; return;}   // detected non indexed by bbhash
        if (this->_fingerprint_size>0 and not this->_prob_set->exists(index, key))  {exists = false; return;}   // bad fingerprint
        exists = true;
        value=this->_values[index];
    }
    
    
    void save(std::ostream& os){
        // save the qd
        quasidictionary<Keys,ValuesType>::save(os);

        // save the _values
        serialize(os, _values);
    }

    
    void load(std::istream& is){
        // read the qd
        quasidictionary<Keys,ValuesType>::load(is);

        // read the values
        _values = std::vector< ValuesType > ();
        deserialize(is, _values);
    }

    
private:
    
    
    
    /**
     * @brief _values stores for each indexed element the value associated to a key
     */
    std::vector< ValuesType > _values;
    
    
};


#endif // QUASIdictionary_H
