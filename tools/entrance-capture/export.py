"""Export every queue/event and inspect every Draw/Dispatch. Raw bytes remain in .wpix."""
import json
from pathlib import Path
import sys
import traceback
from capture import initialize, write_json

def first(value):
    return value[0] if isinstance(value, tuple) else value

def main(capture, output):
    initialize()
    import Microsoft.PIX as pix
    import Microsoft.PIX.Extension as ext
    import Microsoft.PIX.Extension.DeviceConnection as dc
    import Microsoft.PIX.Extension.GpuCapture as gc
    import Microsoft.PIX.Extension.GpuCapture.Analysis as ga
    import Microsoft.PIX.Extension.GpuCapture.Resources as gr
    from pix_views import describe_view
    x = gr.PixApiExtensionsGpuCaptureResources
    factory = ext.PixApiExtensions.PixCreateFactory[pix.IPixFactory]()
    document = ext.PixApiExtensions.OpenGpuCaptureDocument[pix.IPixGpuCaptureDocument](factory, str(capture))
    summary = {'capture': capture.name, 'replay': 'not_started', 'events': 0,
               'program_events': 0, 'program_state_ok': 0, 'view_lists_ok': 0,
               'view_errors': 0, 'resource_count': None, 'errors': [],
               'raw_data': 'Complete PIX capture retained; JSON is an index, not raw GPU payload.',
               'requirements': 'Semantic validation remains required; event names are not proof.'}
    analysis = None
    resources = {}
    try:
        summary['resource_count'] = int(gc.PixApiExtensionsGpuCapture.GetD3D12Resources(document).GetCount())
        analysis = gc.PixApiExtensionsGpuCapture.GetAnalysis(document)
        ga.PixApiExtensionsGpuCaptureAnalysis.Connect(analysis, dc.PIX_CONNECTION_DESC.CreateLocal(), None)
        ga.PixApiExtensionsGpuCaptureAnalysis.StartAnalysis(analysis)
        summary['replay'] = 'started'
        queues = gc.PixApiExtensionsGpuCapture.GetQueues(document)
        with (output / 'events.jsonl').open('w', encoding='utf-8') as stream:
            for qi in range(queues.GetCount()):
                q = first(ext.PixApiExtensions.Get[pix.IPixGpuCaptureQueueInfo](queues, qi))
                for ei in range(q.GetEventCount()):
                    row = {'queue': qi, 'event': ei, 'queue_type': str(q.GetType()),
                           'queue_name': str(q.GetName()), 'adapter': str(q.GetAdapterName())}
                    try:
                        event = gc.PixApiExtensionsGpuCapture.GetEvent(q, ei)
                        row.update(name=str(event.Name), api=str(event.ApiCallData))
                        summary['events'] += 1
                        if row['name'].startswith(('Draw', 'Dispatch')):
                            summary['program_events'] += 1
                            state = first(gc.PixApiExtensionsGpuCapture.GetProgramState(document, event))
                            row['program_type'] = str(x.GetGpuProgramType(state))
                            program = first(x.GetGpuProgram[pix.IPixGpuProgram](state))
                            root = x.GetGlobalRootSignature(program)
                            if root:
                                row['root_signature'] = {'id': int(root.GetApiObjectId()), 'name': str(root.GetName())}
                            summary['program_state_ok'] += 1
                            # The documented generic-pipeline API covers draw/compute.
                            # DXR state/raw shader tables remain in .wpix; failure is explicit.
                            try:
                                pipeline = first(x.GetGpuProgram[pix.IPixGenericPipeline](state))
                                views = x.GetResourceViews(pipeline)
                                row['views'] = []
                                for vi in range(views.GetCount()):
                                    v = {'index': vi}
                                    try:
                                        view = first(x.GetResourceView[pix.IPixResourceView](views, vi))
                                        v['type'] = str(view.GetType())
                                        v['description'] = describe_view(views, vi, view.GetType())
                                        non_resource = ('SAMPLER', 'DESCRIPTOR_RANGE', 'ROOT_CONSTANT', 'GPU_DESCRIPTOR_HANDLE', 'RESOURCE_NONE')
                                        if not any(s in v['type'] for s in non_resource):
                                            rv = first(x.GetResourceView[pix.IPixD3D12ResourceView](views, vi))
                                            resource = x.GetD3D12Resource(rv)
                                            rid = int(resource.GetApiObjectId())
                                            v['resource_id'] = rid
                                            if rid not in resources:
                                                desc = x.GetDesc(resource)
                                                resources[rid] = {'id': rid, 'name': str(resource.GetName()),
                                                    'dimension': str(desc.Dimension), 'width': int(desc.Width),
                                                    'height': int(desc.Height), 'depth_or_array': int(desc.DepthOrArraySize),
                                                    'mips': int(desc.MipLevels), 'format': str(desc.Format),
                                                    'flags': str(desc.Flags)}
                                    except Exception as error:
                                        v['error'] = str(error)
                                        summary['view_errors'] += 1
                                    row['views'].append(v)
                                summary['view_lists_ok'] += 1
                            except Exception as error:
                                row['view_export_error'] = str(error)
                    except Exception as error:
                        row['error'] = str(error)
                    stream.write(json.dumps(row, ensure_ascii=False) + '\n')
                    if ei % 100 == 0:
                        stream.flush()
                        write_json(output / 'summary.json', summary)
        summary['replay'] = 'completed'
    except Exception:
        summary['errors'].append(traceback.format_exc())
        summary['replay'] = 'failed'
    finally:
        if analysis is not None:
            for method in ('StopAnalysis', 'Disconnect'):
                try:
                    getattr(analysis, method)()
                except Exception as e:
                    summary['errors'].append(method + ': ' + str(e))
        summary['indexed_resources'] = len(resources)
        summary['index_complete'] = (summary['replay'] == 'completed' and summary['events'] > 0
            and summary['program_events'] > 0 and summary['view_errors'] == 0
            and summary['view_lists_ok'] == summary['program_events'])
        write_json(output / 'resources.json', list(resources.values()))
        write_json(output / 'summary.json', summary)
    return 0 if summary['replay'] == 'completed' else 1

if __name__ == '__main__':
    sys.exit(main(Path(sys.argv[1]).resolve(), Path(sys.argv[2]).resolve()))
