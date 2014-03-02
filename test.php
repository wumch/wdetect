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
    }

    public function detect($imgFile)
    {
        $detecter = new WDetecter;
        $prepareOpts = (array)(new PrepareOpts);
        $prepareOpts['img_file'] = $imgFile;
        print_r($prepareOpts);
        $prepareRes = $detecter->prepare($prepareOpts);
        print_r($prepareRes);
        echo str_repeat('-', 40), PHP_EOL;
        if ($prepareRes[0] === WDetecter::OK)
        {
            $locateOpts = (array)(new ChartOpts);
            print_r($locateOpts);
            $locateRes = $detecter->locate($locateOpts);
            print_r($locateRes);
            echo str_repeat('-', 40), PHP_EOL;
            if ($locateRes[0] === WDetecter::OK)
            {
                $songdaROC = new SongdaROC;
                $songdaROC->adjust($locateRes[2][0], $locateRes[2][1], 2);
                $detectOpts = (array)($songdaROC);
                print_r($detectOpts);
                $detectRes = $detecter->detect($detectOpts);
                print_r($detectRes);
                echo str_repeat('-', 40), PHP_EOL;
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
{
    public $type;
}

class ChartOpts extends LocateOpts
{
    public $type = WDetecter::CMD_LOCATE_CHART;
    public $chart_min_width = 130;
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

    public function reset($left, $right, $top, $bottom, $x0 = 0, $y0 = 0)
    {
        $this->left = $left + $x0;
        $this->right = $right + $x0;
        $this->top = $top + $y0;
        $this->bottom = $bottom + $y0;
    }
}

class NumOpts extends DIBOpts
{
    public $digit_height = 9;
    public $circle_min_diameter_v = 2;
    public $vline_adj = 0, $hline_adj = 0;
    public $vline_max_break = 0, $hline_max_break = 0;
    public $vline_min_gap = 2, $hline_min_gap = 2;
}

class IntegerOpts extends NumOpts
{
    public $type = WDetecter::CMD_DETECT_INTEGER;
    public $comma_width = 3, $comma_height = 4,
           $comma_min_area = 6, $comma_protrude = 2;
}

class PercentOpts extends NumOpts
{
    public $type = WDetecter::CMD_DETECT_PERCENT;
    public $percent_width = 11;
    public $dot_width = 2, $dot_height = 2,
           $dot_min_area = 4;
}

class SongdaROC extends IntegerOpts
{
    public function adjust($x0, $y0, $padding = 0)
    {
        $this->right = $x0 + 335;
        $this->left = $this->right - 60;
        $this->top = $y0 + 3;
        $this->bottom = $this->top + $this->digit_height;

        $this->left -= $padding;
        $this->top -= $padding;
        $this->right += $padding;
        $this->bottom += $this->comma_protrude + $padding;
    }
}

if ($argc)
{
    $imgFile = count($argv) > 1 ? $argv[1] : "/data/fsuggest/wdetect.forgiven/data/1.jpg";
    $detecter = new Detecter;
    $detecter->check();
    $detecter->detect($imgFile);
}

