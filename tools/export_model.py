"""Reproduce the bundled model using the upstream EfficientAT checkout.

Build dependencies (not required by the app): torch, torchvision, torchaudio, onnx.
Run: python tools/export_model.py /path/to/EfficientAT
The upstream checkout must contain resources/mn04_as_mAP_432.pt. Downloads are
intentionally not performed by this script. Source and weight URLs: models/README.md.
"""
import csv,json,os,sys
from pathlib import Path
import numpy as np
import torch

def main():
    checkout=Path(sys.argv[1]).resolve(); root=Path(__file__).resolve().parent.parent/'models'
    os.chdir(checkout); sys.path.insert(0,str(checkout))
    from models.mn.model import get_model
    from models.preprocess import AugmentMelSTFT
    import torchaudio
    model=get_model(width_mult=.4).eval()
    model.load_state_dict(torch.load('resources/mn04_as_mAP_432.pt',map_location='cpu',weights_only=True),strict=True)
    x=torch.randn(1,1,128,1000)
    torch.onnx.export(model,x,str(root/'efficientat.onnx'),input_names=['mel'],output_names=['logits','embedding'],
                      dynamic_axes={'mel':{3:'frames'}},opset_version=17,dynamo=False)
    pre=AugmentMelSTFT().eval()
    bank,_=torchaudio.compliance.kaldi.get_mel_banks(128,1024,32000,0.,15000.,100.,-500.,1.)
    bank=torch.nn.functional.pad(bank,(0,1)).numpy()
    np.savez(root/'preprocess.npz',mel=bank,window=pre.window.numpy())
    with open('metadata/class_labels_indices.csv',encoding='utf-8') as f: labels=[r['display_name'] for r in csv.DictReader(f)]
    (root/'labels.json').write_text(json.dumps(labels,indent=2),encoding='utf-8')
    print('Exported to',root)
if __name__=='__main__': main()
