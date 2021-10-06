var CircCount = 0;
const HandledCircles = [];
function CirclePropertyHandler(nameOfCircle) {
    this.id = nameOfCircle;
    this.NeedToMap = false;
    this.PercentMap = {
        'min': 0,
        'max': 100,
        'new_min': 0,
        'new_max': 100
    };
    this.RainbowBar = false;
    this.RainbowNumber = false;
    this.Rainbow = { 'min': '#000000', 'max': '#FFFFFF' };
    this.SetNew = function (value) {
        _val = parseInt(value);
        var _color = undefined;
        var _numberColor = undefined;
        var _valueText = undefined;
        if (_val < 0 || _val > 100) {
            console.log('cant parse new value.', this.id, value);
        }
        if (this.NeedToMap) {
            if (Number.isNaN(this.PercentMap.min) || Number.isNaN(this.PercentMap.max) || Number.isNaN(this.PercentMap.new_min) || Number.isNaN(this.PercentMap.new_max)) {
                this.NeedToMap = false;
                console.log('NaN detected in Mapping ' + this.id + ' disabling map functionality');
                return;
            }
            _val = mapN(_val, this.PercentMap.min, this.PercentMap.max, this.PercentMap.new_min, this.PercentMap.new_max);
            _valueText = value;
        }
        if (this.RainbowBar) {
            //TODO get raibow color
            _color = "#FFFFF0";
        }
        if (this.RainbowNumber) {
            //TODO get raibow color
            _numberColor = "#FFFFF0";
        }

        setnew(this.id, _val, _valueText, _color, _numberColor);
    };

}
///Circles
function generateCircle(div, _title, _initialPercent, _id, _bgcolor, _color, _numbercolor, _sign, args) {
    if (typeof div === 'object')
        div.innerHTML = createCircle(_title, _initialPercent, _id, _bgcolor, _color, _numbercolor, _sign);
    else {
        document.getElementById(div).innerHTML = createCircle(_title, _initialPercent, _id, _bgcolor, _color, _numbercolor, _sign);
    }

    var _property = new CirclePropertyHandler(_id);

    if (typeof args !== 'undefined') {
        _args = args.split(',');


        for (var i = 0; i < _args.length; i++) {
            _args2 = _args[i].split(' ');
            if (_args2[0] == 'map') {
                _property.NeedToMap = true;
                _property.PercentMap.min = parseInt(_args2[1]);
                _property.PercentMap.max = parseInt(_args2[2]);
                _property.PercentMap.new_min = parseInt(_args2[3]);
                _property.PercentMap.new_max = parseInt(_args2[4]);
            }
            else if (_args2[0] == 'rainbow') {
                _property.RainbowBar = true;
                _property.RainbowTitle = true;
                _property.Rainbow.min = _args2[1];
                _property.Rainbow.max = _args2[2];
            }
        }
    }

    HandledCircles.push(_property);
}
function createCircle(_title, _percent, _Id, _bgcolor, _color, _numbercolor, _sign) {
    if (typeof _title === 'undefined') {
        _title = null;
    }
    if (typeof _percent === 'undefined') {
        _percent = 0;
    }
    if (typeof _Id === 'undefined') {
        _Id = 'newId';

    }
    if (typeof _bgcolor === 'undefined') {
        _bgcolor = "#d4ebd3";
    }
    if (typeof _color === 'undefined') {
        _color = "darkred";
    }
    if (typeof _numbercolor === 'undefined') {
        _numbercolor = "#5f6362";
    }
    if (typeof _sign === 'undefined') {
        _sign = '%';
    }

    var percent = 440 * _percent / 100;
    percent = 440 - percent;
    var rtr = `
<div class="circle_box">
<h2 class="legend">` + _title + `</h2>
<div class="circle_Pbar">
<svg id= "`+ _Id+`_svg">
<circle style="stroke:` + _bgcolor + `;"cx ="70" cy="70" r="70" > </circle>
<circle class="c` + CircCount + `" id="` + _Id + `" cx="70" cy="70" r="70">
<style>
.circle_box .circle_Pbar .c`+ CircCount + ` {

stroke:`+ _color + `;
stroke-dasharray: 440;
stroke-dashoffset:`+ percent + `;
animation: mymove`+ CircCount + ` 2s ease-in;
animation-iteration-count: 1;
}
@keyframes mymove`+ CircCount + `{
from {stroke-dashoffset: 440;}
to {stroke-dashoffset:` + percent + `;}
}</style>
</circle>
</svg>
<div class="circle_prog" style="color:`+ _numbercolor + `;"> <h2 id="` + _Id + "N" + `"> ` + _percent + ` </h2><span>` + _sign + `</span></div></div>
</div>`;

    CircCount++;
    return rtr;
}
function setnew(_id, _newPercent, _newNumber, _color, _titleColor, ) {
    var _idN = _id + "N";
    if (typeof _color !== 'undefined') {
        document.getElementById(_id).style.stroke = _color;
    }
    if (typeof _titleColor !== 'undefined') {
        document.getElementById(_idN).style.color = _titleColor;
    }
    if (typeof _newNumber === 'undefined')
        document.getElementById(_idN).innerHTML = _newPercent;
    else
        document.getElementById(_idN).innerHTML = _newNumber;

    var _newoffset = 440 - (440 * _newPercent / 100);
    var _foward = true;
    if (_newoffset > parseInt(getComputedStyle(document.getElementById(_id)).strokeDashoffset, 10))
        _foward = false;

    //            console.log(parseInt(getComputedStyle(document.getElementById(_id)).strokeDashoffset, 10), _newoffset, _newPercent, _newNumber, _foward)
    if (!document.hidden)
        animateRoute(document.getElementById(_id), parseInt(getComputedStyle(document.getElementById(_id)).strokeDashoffset, 10), _newoffset, _foward);
}
function animateRoute(_id, _start, _end, _foward) {
    if (_foward) {
        _start -= 1;
        if (_start <= _end) {
            _start = _end;
            _id.style.strokeDashoffset = _start;
        }
        else {
            _id.style.strokeDashoffset = _start;
            setTimeout(function () { animateRoute(_id, _start, _end, true); }, 10);
        }
    }
    else {
        _start += 1;
        if (_start >= _end) {
            _start = _end;
            _id.style.strokeDashoffset = _start;
        }
        else {
            _id.style.strokeDashoffset = _start;
            setTimeout(function () { animateRoute(_id, _start, _end, false); }, 10);
        }
    }

}
function mapN(value, in_min, in_max, out_min, out_max) {
    if (value > out_max)
        return out_max;
    if (value < out_min)
        return out_min;
    return (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

function setnewangle(id, angle)
{
        document.getElementById(id + "_svg").style.transform ="rotate("+angle+"deg)";     
}

function setLabelSize(id, size) {
    id = id+"N";
    document.getElementById(id).style.fontSize = size;
    
}
/// REturns [minIncrement, convertTime]
function ds18(resolution) {

    resolution = resolution - 9;
    var data = [];
    resolution = Math.pow(2, resolution);
    data[0] = 0.5 / resolution;  
    data[1] = 93.75 * resolution;
    console.log(data);
    return data;

    
}