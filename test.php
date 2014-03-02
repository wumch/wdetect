<?php

$moduleName = 'wdetect';
$className = 'WDetecter';

if (!extension_loaded($moduleName))
{
    die("module <{$moduleName}> is not loaded");
}
else if (!class_exists($className))
{
    die("module <{$moduleName}> loaded, but class <{$className}> still non exists");
}

class Detecter
{
    protected $detecter;

    protected $prepareOptions = array(
        'img_file_min_size' => 24,
        'img_file_max_size' => 8388608, // 8MB
        'img_min_width' => 160,
        'img_max_width' => 1920,
        'img_min_height' => 120,
        'img_max_height' => 1280,
    );

    protected $locateOptions = array(
        'type' => WDetecter::CMD_LOCATE_CHART,
        'chart_min_width' => 100,
        'chart_max_width' => 170,
        'chart_min_height' => 32,
        'chart_max_height' => 132,
        'echelons' => 3,
        'echelon_padding_left' => 1,
    );

    protected $detectOptions = array(
        'type' => WDetecter::CMD_DETECT_INTEGER,
    );

    public function __construct()
    {
        $this->detecter = new WDetecter;
    }

    public function detect($imgfile)
    {
        $prepareRes = $this->detecter->prepare($this->prepareOptions);
        var_dump($prepareRes);
        if ($prepareRes)
        {
            $locateRes = $this->detecter->locate($this->locateOptions);
            var_dump($locateRes);
            if ($locateRes)
            {
                $detectRes = $this->detecter->detect($this->detectOptions);
                var_dump($detectRes);
                if ($detectRes)
                {
                    echo 'done', PHP_EOL;
                }
            }
        }
    }

    public static function check()
    {
        assert(WDetecter::OK == 0);
        assert(WDetecter::ERR_UNKNOWN == 1);
    }
}

class Options
{}

class PrepareOpts extends Options
{
    public $img_file = "";
    public $img_file_min_size = 24;
    public $img_file_max_size = 8388608;
    public $img_min_width = 160;
    public $img_max_width = 1920;
    public $img_min_height = 120;
    public $img_max_height = 1280;
}

class LocateOpts extends Options
{}

class ChartOpts extends LocateOpts
{
    public $chart_min_width = 120;
    public $chart_max_width = 170;
    public $chart_min_height = 32;
    public $chart_max_height = 132;
    public $echelons = 3;
    public $echelon_padding_left = 1;
}

class DetectOpts extends Options
{
    public $type;
    public $x_err = 1, $y_err = 1;
}

class DIBOpts extends DetectOpts
{
    public $left, $top, $right, $bottom;
}

class NumOpts extends DIBOpts
{
    public $digit_height;
    public $circle_min_diameter_v;
    public $vline_adj = 0, $hline_adj = 0;
    public $vline_max_break = 0, $hline_max_break = 0;
    public $vline_min_gap = 2, $hline_min_gap = 2;
}

class IntegerOpts extends NumOpts
{
    public $comma_width = 3, $comma_height = 4,
           $comma_min_area = 6, $comma_protrude = 2;
}

class PercentOpts extends NumOpts
{
    public $percent_width = 11;
    public $dot_width = 2, $dot_height = 2,
           $dot_min_area = 4;
}

print_r((array)(new NumOpts));

if ($argc)
{
    $imgFile = count($argv) > 1 ? $argv[1] : "/data/fsuggest/wdetect.forgive/data/1.jpg";
    $detecter = new Detecter;
    $detecter->check();
    $detecter->detect($imgFile);
}

