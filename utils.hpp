#pragma once


template<size_t N>
class TrendCalc {
public:

    void reset() {
        nx = -1;
    }

    void add(int v) {
        if (nx < 0) {
            for (auto &x: values) x = v;
            nx = 0;
        } else {
            values[nx] = v;
            nx++;
            if (static_cast<size_t>(nx) >= N) nx = 0;
        }
    }

    float slope(size_t points) {
        if (points > N) points = N;
        if (points < 2) points = 2;
        int start = (nx +  N - points) % N;

        float sum_x = 0, sum_y = 0, sum_xy = 0, sum_xx = 0;

        for (size_t i = 0; i < points; ++i) {
            int val = values[(start + i) % N];
            sum_x += i;
            sum_y += val;
            sum_xy += static_cast<int>(i) * val;
            sum_xx += i * i;            
        }

        float trend = (points * sum_xy - sum_x * sum_y) / (points * sum_xx - sum_x * sum_x); 
        return trend;        
    }

protected:
    int values[N] = {};
    int nx = -1;
};



class CidloRead {
public:

    CidloRead(int rx, int tx):sr(rx, tx) {}
    void begin() {
        sr.begin(9600);
    }
    bool read() {
        if (sr.available()) {
            int b = sr.read();
            if (_cntr > 0 || b == 0xFF) {
                _data[_cntr] = b;
                ++_cntr;
                if (_cntr == 4) {
                   _cntr = 0;
                    int chks = (_data[0]+_data[1]+_data[2]) & 0xFF;
                    if (chks == _data[3]) {
                        _avgdata[_npos] = _data[1]*256+_data[2];
                        _npos ++;
                        _npos = _npos & 0xF;
                        return true;
                    }
                }             
            }
        }
        return false;
    }

    int get_value() const {
        int sum = 0;
        for (const int &v: _avgdata) sum+=v;
        return sum >> 4;
    }

protected:
    SoftwareSerial sr;
    int _data[4];
    int _cntr = 0;    
    int _avgdata[16];
    int _npos = 0;
};
