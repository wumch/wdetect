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
    protected $error = array(
        WDetecter::ERR_UNKNOWN => '未知错误',
        WDetecter::ERR_WRONG_PARAM => '参数错误',
        WDetecter::ERR_IMG_FILE_NONEXISTS => '图片文件不存在',
        WDetecter::ERR_IMG_FILE_UNREADABLE => '无法读取图片文件内容',
        WDetecter::ERR_IMG_FILE_EXT => '不支持的图片文件类型（扩展名）',
        WDetecter::ERR_IMG_FILE_SIZE => '图片文件体积不合格',
        WDetecter::ERR_IMG_TYPE => '不支持的图片文件类型（文件头）',
        WDetecter::ERR_IMG_CONTENT => '图片文件内容有误',
        WDetecter::ERR_IMG_SIZE => '图片尺寸不合格',
        WDetecter::ERR_NO_MATCH => '找不到符合条件的图像',
        WDetecter::ERR_DETECT_FAILED => '探测失败',
        WDetecter::ERR_RECOGNIZE_FAILED => '识别失败',
    );

    public function __construct()
    {
    }

    public function detect($imgFile)
    {
        global $className;
        $detecter = new $className;
        $prepareOpts = (array)(new PrepareOpts);
        $prepareOpts['img_file'] = $imgFile;
        print_r($prepareOpts);
        $prepareRes = $detecter->prepare($prepareOpts);
        print_r($prepareRes);
        echo str_repeat('-', 40), PHP_EOL;
        if ($prepareRes[0] === $className::OK)
        {
            $locateOpts = (array)(new ChartOpts);
            print_r($locateOpts);
            $locateRes = $detecter->locate($locateOpts);
            print_r($locateRes);
            echo str_repeat('-', 40), PHP_EOL;
            if ($locateRes[0] === $className::OK)
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
        global $className;
        assert($className::OK == 0);
        assert($className::ERR_UNKNOWN == 1);
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
    public $echelon_padding_left = 0;
}

class DetectOpts extends Options
{
    public $type;
    public $x_err = 1, $y_err = 1;
    public $inverse = false;
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
    public $digit_min_width = 4;
    public $digit_max_width = 7;
    public $circle_min_diameter_v = 2;
    public $vline_adj = 0, $hline_adj = 0;
    public $vline_max_break = 0, $hline_max_break = 0;
    public $vline_min_gap = 2, $hline_min_gap = 2;
}

class IntegerOpts extends NumOpts
{
    public $type = WDetecter::CMD_DETECT_INTEGER;
    public $comma_max_width = 3, $comma_max_height = 4,
           $comma_min_area = 3, $comma_protrude = 2;
}

class PercentOpts extends NumOpts
{
    public $type = WDetecter::CMD_DETECT_PERCENT;
    public $percent_width = 11;
    public $dot_max_width = 2, $dot_max_height = 2,
           $dot_min_area = 2;
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

